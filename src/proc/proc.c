#include "proc.h"

#include <dirent.h>
#include <sys/types.h>
#include <ctype.h>

static unsigned long long cur_gen = 0;

static int cond(process_data_t* data) {
    return data->gen == cur_gen;
}

void refresh_processes(processes_map_t* m, unsigned long long cpu_total_delta) {
    cur_gen++;

    DIR* dir = opendir("/proc");

    struct dirent* ent;
    while((ent = readdir(dir))) {

        const char* c = ent->d_name;
        pid_t pid = 0;
        for(; *c ; c++) {
            if(!isdigit((unsigned char)*c)) {pid = -1; break;}
            pid = pid * 10 + (*c - '0');
        }

        if(pid <= 0) continue;
        if(m->size >= m->capacity) break;

        process_data_t p = {0};
        p.pid = pid;


        /*todo Read stats*/


        process_data_t* existing = get_process(m, pid);
        if(existing) {
            existing->rss_kb  = p.rss_kb;
            existing->cpu_pct = p.cpu_pct;
            existing->pss_kb  = p.pss_kb;
            existing->state   = p.state;

            existing->gen = cur_gen;
        }
        else {
            p.gen = cur_gen;
            insert_proc(m, p); 
        }
    }

    closedir(dir);
    filter_map(m, cond);
}

