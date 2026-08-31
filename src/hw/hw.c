#include "hw.h"

#include <stddef.h>
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

    char gpu_name[256];
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

static void lookup_pci_name(unsigned int vid, unsigned int did, char* out, size_t out_len) {

    const char* paths[] = {
        "/usr/share/misc/pci.ids",
        "/usr/share/hwdata/pci.ids",
        "/usr/share/pci.ids",
        NULL
    };

    FILE* f = NULL;
    for (int i = 0; paths[i]; i++) {

        f = fopen(paths[i], "r");
        if (f) break;
    }

    if (!f) { snprintf(out, out_len, "Unknown GPU"); return; }

    char line[256];
    int in_vendor = 0;

    while (fgets(line, sizeof(line), f)) {

        if (line[0] == '#' || line[0] == '\n') continue;

        if (line[0] != '\t') {

            unsigned int v;
            if (sscanf(line, "%x", &v) == 1) in_vendor = (v == vid);

        } else if (in_vendor && line[1] != '\t') {
            unsigned int d;
            char name[200];

            if (sscanf(line, " %x %199[^\n]", &d, name) == 2 && d == did) {

                snprintf(out, out_len, "%s", name);

                fclose(f);
                return;
            }
        }
    }

    fclose(f);
    snprintf(out, out_len, "Unknown GPU");
}

static void fetch_gpu_name(hw_monitor_t* hw) {

    for (int i = 0; i < 4; i++) {
        char uevent_path[MAX_PATH_LEN];
        snprintf(uevent_path, MAX_PATH_LEN, "/sys/class/drm/card%d/device/uevent", i);

        FILE* f = fopen(uevent_path, "r");
        if (!f) continue;

        char line[256];
        unsigned int vid = 0, did = 0;
        int found = 0;

        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "PCI_ID=%x:%x", &vid, &did) == 2) {
                found = 1;
                break;
            }
        }
        fclose(f);

        if (!found) continue;

        lookup_pci_name(vid, did, hw->gpu_name, sizeof(hw->gpu_name));

        char* open  = strchr(hw->gpu_name, '[');
        char* close = strrchr(hw->gpu_name, ']');

        if(open && close && close > open) {

            size_t len = close - (open + 1);
            memmove(hw->gpu_name, open + 1, len);

            hw->gpu_name[len] = '\0';
        }

        hw->gpu_name_ready = 1;
        return;
    }

    snprintf(hw->gpu_name, sizeof(hw->gpu_name), "Unknown GPU");
    hw->gpu_name_ready = 1;

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
    fetch_gpu_name(hw);

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

void free_hw_monitor(hw_monitor_t* hw) {
    free(hw);
}
