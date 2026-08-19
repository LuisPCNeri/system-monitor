#include "hw.h"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

typedef struct hw_monitor_t {

    char cpu_temp_path[MAX_PATH_LEN];
    char gpu_temp_path[MAX_PATH_LEN];
    int cpu_found;
    int gpu_found;

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

hw_monitor_t* init_hw_monitor() {
    hw_monitor_t* hw = (hw_monitor_t*) malloc(sizeof(hw_monitor_t));

    find_thermal_zones(hw);
    find_hwmon_entries(hw);

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

float read_gpu_temp(hw_monitor_t *hw) {
    if(!hw->gpu_found) return -1.0f;

    FILE* f = fopen(hw->gpu_temp_path, "r");
    if(!f) return -1.0f;

    int millideg;
    fscanf(f, "%d", &millideg);

    fclose(f);

    return millideg / 1000.0f;
}
