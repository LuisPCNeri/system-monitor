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

/*
 *  \brief Reads /proc/stat to get the cpu data.
 *  \param out A pointer to a CpuStat struct to store the data.
 *  \returns 0 on success 1 on failure.
 * */
int cpu_read(CpuStat* out);

/*
 *  \brief Calculates the cpu usage pct based on two different CpuStat structs sepparated by a time difference.
 *  \param prev Old CpuStat data.
 *  \param curr Newer CpyStat data.
 *  \returns The current cpu usage pct.
 * */
float cpu_usage(CpuStat* prev, CpuStat* curr);

unsigned long long cpu_total_delta(CpuStat* prev, CpuStat* curr);

#endif
