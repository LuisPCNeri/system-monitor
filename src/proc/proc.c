#include "proc.h"

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include <stdlib.h>

static unsigned long long cur_gen = 0;

static int cond(process_data_t* data) {
    return data->gen == cur_gen;
}

static int read_stat(process_data_t* p) {
        char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", p->pid);
 
    FILE *f = fopen(path, "r");
    if (!f) return -1;
 
    char buf[1024];
    int ok = (fgets(buf, sizeof(buf), f) != NULL);
    fclose(f);
    if (!ok) return -1;
 
    /* Locate name boundaries */
    char *name_start = strchr(buf, '(');
    char *name_end   = strrchr(buf, ')');
    if (!name_start || !name_end || name_end <= name_start) return -1;
 
    int name_len = (int)(name_end - name_start) - 1;
    if (name_len >= (int)sizeof(p->name)) name_len = (int)sizeof(p->name) - 1;
    memcpy(p->name, name_start + 1, (size_t)name_len);
    p->name[name_len] = '\0';
 
    /* Parse fields that come after the closing ')' */
    char state;
    int  ppid, pgrp, session, tty, tpgid;
    unsigned int flags;
    unsigned long long minflt, cminflt, majflt, cmajflt;
    unsigned long long utime, stime;
 
    int r = sscanf(name_end + 2,
        "%c %d %d %d %d %d %u "
        "%llu %llu %llu %llu "
        "%llu %llu",
        &state, &ppid, &pgrp, &session, &tty, &tpgid, &flags,
        &minflt, &cminflt, &majflt, &cmajflt,
        &utime, &stime);
 
    if (r < 13) return -1;
 
    p->state = state;
    p->utime = utime;
    p->stime = stime;
    return 0;
}

static int read_rss(process_data_t* p) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/statm", p->pid);

    FILE* f = fopen(path, "r");
    if(!f) return -1;

    long size_pages, rss_pages;
    int r = fscanf(f, "%ld %ld", &size_pages, &rss_pages);
    fclose(f);
    if(r < 2) return  -1;

    long page_bytes = sysconf(_SC_PAGESIZE);
    p->rss_kb = (rss_pages * page_bytes) / 1024;

    return 0;
}

static int read_pss(process_data_t* p) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", p->pid);

    FILE* f = fopen(path, "r");
    if(!f) return -1;

    char line[256];
    long pss_kb = -1;

    while(fgets(line, sizeof(line), f)) {
        if(strncmp(line, "Pss:", 4) == 0) {
            sscanf(line, "Pss: %ld kB", &pss_kb);
            break;
        }
    }

    p->pss_kb = pss_kb == -1 ? 0 : pss_kb;

    fclose(f);
    return 0;
}

static int read_process_uid(process_data_t* p) {

    char fpath[64];
    snprintf(fpath, sizeof(fpath), "/proc/%d", p->pid);

    struct stat s;
    if(stat(fpath, &s) == 0) {
        return s.st_uid;
    };

    return -1;
}

static char* resolve_user_with_uid(int uid) {

    static char uname[32];
    FILE *f = fopen("/etc/passwd", "r");
    if (!f) return NULL;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *colon1 = strchr(line, ':');
        if (!colon1) continue;
        char *colon2 = strchr(colon1 + 1, ':');
        if (!colon2) continue;

        if (atoi(colon2 + 1) == uid) {
            size_t len = colon1 - line;
            if (len >= sizeof(uname)) len = sizeof(uname) - 1;

            memcpy(uname, line, len);
            uname[len] = '\0';

            fclose(f);
            return uname;
        }
    }

    fclose(f);
    return NULL;
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
        if((int) m->size >= m->capacity) break;

        process_data_t p = {0};
        p.pid = pid;

        if (read_stat(&p) < 0) continue;
        read_rss(&p);
        read_pss(&p);

        int uid = read_process_uid(&p);
        if( uid >= 0) {
            char* name = resolve_user_with_uid(uid);
            if(name) snprintf(p.user, sizeof(p.user), "%s", name);
        }

        process_data_t* existing = get_process(m, pid);
        if(existing) {
            if(cpu_total_delta > 0) {
                unsigned long long proc_delta = (p.utime + p.stime) - (existing->utime + existing->stime);

                long num_cores = sysconf(_SC_NPROCESSORS_ONLN);

                p.cpu_pct = 100.0f * (float) proc_delta / (float) cpu_total_delta;
                p.irix_cpu_pct = (100.0f * (float) proc_delta / (float) cpu_total_delta) * num_cores;

            }

            *existing = p;
            existing->gen = cur_gen;
        }
        else {
            p.cpu_pct = 0.0f;
            p.gen = cur_gen;
            insert_proc(m, p); 
        }
    }

    closedir(dir);
    filter_map(m, cond);
}

void kill_process(pid_t pid) {
    kill(pid, SIGKILL);
}

int proc_cmp_cpu(const void *a, const void *b) {
    float da = ((const process_data_t*)a)->cpu_pct;
    float db = ((const process_data_t*)b)->cpu_pct;
    return (db > da) - (db < da);
}

int proc_cmp_mem(const void *a, const void *b) {
    long da = ((const process_data_t*)a)->rss_kb;
    long db = ((const process_data_t*)b)->rss_kb;
    return (db > da) - (db < da);
}

