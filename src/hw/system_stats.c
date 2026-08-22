#include "system_stats.h"

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

typedef struct system_stats {

    char power_file[512];
    short power_found;

} system_stats;

static void find_power_dir(system_stats* st) {

    DIR* dir = opendir("/sys/class/power_supply");

    struct dirent* entry;
    while((entry = readdir(dir)) != NULL) {

        if(strncmp(entry->d_name, "BAT", (size_t) 3) != 0) continue;

        char fpath[512];
        snprintf(fpath, sizeof(fpath), "/sys/class/power_supply/%s/power_now", entry->d_name);
        FILE* f = fopen(fpath, "r");

        if(!f) continue;

        long power_draw;
        if( fscanf(f, "%ld", &power_draw) == 1) {
            strncpy(st->power_file, fpath, sizeof(st->power_file));
            st->power_found = 1;
        }

        fclose(f);
    }

    closedir(dir);
}

system_stats* init_st() {

    system_stats* st = (system_stats*) malloc(sizeof(system_stats));
    if(!st) return NULL;

    find_power_dir(st);
    return st;
}


float read_system_uptime_sec() {

    FILE* f = fopen("/proc/uptime", "r");
    if(!f) return -1.0f;

    float uptime_sec;
    fscanf(f, "%f", &uptime_sec);

    fclose(f);

    return uptime_sec;
}

float read_system_avg_load_5() {

    FILE* f = fopen("/proc/loadavg", "r");
    if(!f) return -1.0f;

    float load_avg_5;
    fscanf(f, "%*f %f", &load_avg_5);

    fclose(f);

    return load_avg_5;
}

float read_power_draw_watts(system_stats* st) {

    FILE* f = fopen(st->power_file, "r");
    if(!f) return -1.0f;

    long power_draw;
    fscanf(f, "%ld", &power_draw);

    fclose(f);

    return (float) power_draw / 1000000.0f;
}
