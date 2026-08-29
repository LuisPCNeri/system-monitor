#include "hw.h"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>

typedef struct hw_monitor_t {

    char cpu_temp_path[MAX_PATH_LEN];
    char gpu_temp_path[MAX_PATH_LEN];
    int cpu_found;
    int gpu_found;

    char gpu_name[128];
    int gpu_name_ready;

} hw_monitor_t;

static void find_thermal_zones(hw_monitor_t* hw) {
    DIR* d = opendir("/sys/class/thermal");
    if(!d) return;

    struct dirent* entry;
    while((entry = readdir(d)) != NULL) {
        if(strncmp(entry->d_name, "thermal_zone", 12) != 0) continue;

        char type_path[MAX_PATH_LEN];
        snprintf(type_path, MAX_PATH_LEN, "/sys/class/thermal/%s/type", entry->d_name);

        FILE* f = fopen(type_path, "r");
        if(!f) continue;

        char type[64];
        fgets(type, sizeof(type), f);
        fclose(f);

        type[strcspn(type, "\n")] = '\0';
        if(strstr(type, "pkg") || strstr(type, "x86") || strstr(type, "cpu") || strstr(type, "k10")) {
            snprintf(hw->cpu_temp_path, MAX_PATH_LEN, "/sys/class/thermal/%s/temp", entry->d_name);
            hw->cpu_found = 1;
        }
    }

    closedir(d);
}

static void find_hwmon_entries(hw_monitor_t* hw) {
    DIR* d = opendir("/sys/class/hwmon");
    if (!d) return;

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (strncmp(entry->d_name, "hwmon", 5) != 0) continue;

        char name_path[MAX_PATH_LEN];
        snprintf(name_path, MAX_PATH_LEN, "/sys/class/hwmon/%s/name", entry->d_name);

        FILE* f = fopen(name_path, "r");
        if (!f) continue;

        char name[64] = {0};
        fgets(name, sizeof(name), f);
        fclose(f);

        name[strcspn(name, "\n")] = '\0';

        if (strcmp(name, "coretemp") == 0 || strcmp(name, "k10temp") == 0) {
            if(!hw->cpu_found) {
                snprintf(hw->cpu_temp_path, MAX_PATH_LEN,
                         "/sys/class/hwmon/%s/temp1_input", entry->d_name);
                hw->cpu_found = 1;
                printf("[hw] CPU hwmon: %s (%s)\n", entry->d_name, name);
            }
        }
        else if (strcmp(name, "amdgpu") == 0 || strcmp(name, "radeon") == 0 ||
                 strcmp(name, "nouveau") == 0 || strcmp(name, "i915") == 0) {
            snprintf(hw->gpu_temp_path, MAX_PATH_LEN,
                     "/sys/class/hwmon/%s/temp1_input", entry->d_name);
            hw->gpu_found = 1;
            printf("[hw] GPU hwmon: %s (%s)\n", entry->d_name, name);
        }
    }
    closedir(d);
}

void fetch_gpu_name(char* buffer, size_t max_len) {

    snprintf(buffer, max_len, "Unknown GPU");
 
    FILE* f = popen("lspci", "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {

        if (strstr(line, "VGA") || strstr(line, "3D controller") || strstr(line, "Display")) {

            char* first_colon = strchr(line, ':');
            if (first_colon) {

                char* second_colon = strchr(first_colon + 1, ':');
                if (second_colon) {

                    char* name_start = second_colon + 1;

                    char* bracket_start = strchr(name_start, '[');
                    char* bracket_end = NULL;

                    if (bracket_start) {
                        bracket_end = strchr(bracket_start + 1, ']');
                    }

                    if (bracket_start && bracket_end) {
                        size_t len = bracket_end - (bracket_start + 1);
                        if (len >= max_len) len = max_len - 1;

                        strncpy(buffer, bracket_start + 1, len);
                        buffer[len] = '\0';
                    } else {
                        while (*name_start == ' ') name_start++;
                        name_start[strcspn(name_start, "\n")] = '\0';
                        snprintf(buffer, max_len, "%s", name_start);
                    }

                    if (!strstr(line, "Intel") && !strstr(line, "Unknown")) {
                        break;
                    }
                }
            }
        }
    }
    pclose(f);
}


static void* fetch_gpu_name_thread(void* arg) {
    hw_monitor_t* hw = (hw_monitor_t*) arg;
    fetch_gpu_name(hw->gpu_name, sizeof(hw->gpu_name));
    hw->gpu_name_ready = 1;
    return NULL;
}

char* get_gpu_name(hw_monitor_t* hw) {
    if(!hw || !hw->gpu_name_ready) return NULL;
    if(hw->gpu_name[0] == '\0' || hw->gpu_name[0] == '\n') return NULL;

    return hw->gpu_name;
}

hw_monitor_t* init_hw_monitor() {
    hw_monitor_t* hw = (hw_monitor_t*) malloc(sizeof(hw_monitor_t));

    find_thermal_zones(hw);
    find_hwmon_entries(hw);

    snprintf(hw->gpu_name, sizeof(hw->gpu_name), "...");
    pthread_t t;
    pthread_create(&t, NULL, fetch_gpu_name_thread, hw);
    pthread_detach(t);

    return hw;
}

float read_cpu_temp(hw_monitor_t* hw) {
    if(!hw->cpu_found) return -1.0f;

    FILE* f = fopen(hw->cpu_temp_path, "r");
    if(!f) return -1.0f;

    int millideg;
    fscanf(f, "%d", &millideg);

    fclose(f);

    return millideg / 1000.0f;
}

static int nvidia_smi_available = 1;

/// done using nvidia-smi by spawning a sub process. whilst spawning a subprocess is an expensive task on the cpu, this is ran every 1.5 seconds
/// and may be done even less frequently. The other option would be breaking my objective with static compilation, that is to be portable
/// so nvidia-smi will be used in place of dlopen
static int read_nvidia_gpu_temp(void) {
    if(!nvidia_smi_available) return -1;

    FILE* f = popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader 2>/dev/null", "r");
    if(!f) return -1;

    int temp;
    if(fscanf(f, "%d", &temp) == 1) {
        pclose(f);
        return temp;
    }

    pclose(f);
    nvidia_smi_available = 0;
    return -1;
}

float read_gpu_temp(hw_monitor_t *hw) {
    float final_temp = -1.0f;

    if(hw->gpu_found) {

        FILE* f = fopen(hw->gpu_temp_path, "r");
        if(f) {
            int millideg = -1;
            if(fscanf(f, "%d", &millideg) == 1) {
                final_temp = (float) millideg / 1000.0f;
            }

            fclose(f);
        }
    }


    if(final_temp < 0.0f) {
        int nvidia_temp = read_nvidia_gpu_temp();
        if(nvidia_temp >= 0) final_temp = (float) nvidia_temp;
    }

    return final_temp;
}
