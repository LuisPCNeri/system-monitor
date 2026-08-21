#include "system_stats.h"

#include <stdio.h>

float read_system_uptime_sec() {

    FILE* f = fopen("/proc/uptime", "r");
    if(!f) return -1.0f;

    float uptime_sec;
    fscanf(f, "%f", &uptime_sec);

    return uptime_sec;
}

float read_system_avg_load_5() {

    FILE* f = fopen("/proc/loadavg", "r");
    if(!f) return -1.0f;

    float load_avg_5;
    fscanf(f, "%*f %f", &load_avg_5);

    return load_avg_5;
}
