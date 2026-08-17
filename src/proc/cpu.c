#include "cpu.h"
#include <stdio.h>
 
int cpu_read(CpuStat *out) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return -1;
 
    int r = fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
        &out->user, &out->nice, &out->system, &out->idle,
        &out->iowait, &out->irq, &out->softirq, &out->steal);
 
    fclose(f);
    return (r == 8) ? 0 : -1;
}

static unsigned long long total(const CpuStat *s) {
    return s->user + s->nice + s->system + s->idle
         + s->iowait + s->irq + s->softirq + s->steal;
}

static unsigned long long idle_time(const CpuStat *s) {
    return s->idle + s->iowait;
}

float cpu_usage(CpuStat *prev, CpuStat *curr) {
    unsigned long long tot = total(curr)     - total(prev);
    unsigned long long idl = idle_time(curr) - idle_time(prev);
    if (tot == 0) return 0.0f;
    return 100.0f * (1.0f - (float)idl / (float)tot);
}

unsigned long long cpu_total_delta(CpuStat *prev, CpuStat *curr) {
    return total(curr) - total(prev);
}

