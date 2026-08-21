#ifndef __PROC_H__
#define __PROC_H__

#include "../utils/hmap.h"
#include <sys/types.h>

/*
 *  \brief Reconstructs the processes map, updating the stats for existing processes and rewriting dead processes, responsible for getting process data.
 *  \param map The map to refresh.
 *  \param cpu_total_delta The cpu delta to calculate the cpu pct for each process.
 * */
void refresh_processes(processes_map_t* map, unsigned long long cpu_total_delta);

/*
 *  \brief Basically a wrapper function for the kill(__pid_t pid, int sig) function that sends a SIGKILL sig to the process with pid.
 *  \param pid The pid of the process to kill.
 * */
void kill_process(pid_t pid);

/*
 *  \brief Comparison function to use in qsort by cpu pct descending.
 * */
int proc_cmp_cpu(const void *a, const void *b);

/*
 *  \brief Comparison function to use in qsort by RSS descending.
 * */
int proc_cmp_mem(const void *a, const void *b);

#endif
