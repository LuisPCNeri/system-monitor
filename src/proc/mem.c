#include "mem.h"
#include <stdio.h>


int mem_read(mem_info_t *out){
    FILE* f = fopen("/proc/meminfo", "r");
    if(!f) return -1;

    long total = -1, avail = -1;
    char line[128];

    while(fgets(line, sizeof(line), f)) {
        long val;
        if      (sscanf(line, "MemTotal: %ld kB",     &val) == 1) total = val;
        else if (sscanf(line, "MemAvailable: %ld kB", &val) == 1) avail = val;
        if (total >= 0 && avail >= 0) break;
    }

    fclose(f);
 
    if (total < 0 || avail < 0) return -1;
 
    out->total_kb  = total;
    out->used_kb   = total - avail;
    out->total_mib = total / 1024;
    out->used_mib  = (total - avail) / 1024;
    return 0;

}
