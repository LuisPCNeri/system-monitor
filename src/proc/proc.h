#ifndef __PROC_H__
#define __PROC_H__

#include "../utils/hmap.h"

void refresh_processes(processes_map_t* map, unsigned long long cpu_total_delta);

int proc_cmp_cpu(const void *a, const void *b);
int proc_cmp_mem(const void *a, const void *b);

#endif
