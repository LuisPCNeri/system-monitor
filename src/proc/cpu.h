#ifndef _CPU_H_
#define _CPU_H_

#pragma once

typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
} CpuStat;

int cpu_read(CpuStat *out);
float cpu_usage(CpuStat *prev, CpuStat *curr);
unsigned long long cpu_total_delta(CpuStat *prev, CpuStat *curr);

#endif
