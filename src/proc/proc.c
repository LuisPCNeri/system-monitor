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

#define UID_CACHE_SIZE 256

typedef struct {
    uid_t uid;
    char  name[32];
} uid_entry_t;

static uid_entry_t uid_cache[UID_CACHE_SIZE];
static int         uid_cache_len   = 0;
static int         uid_cache_built = 0;

static void build_uid_cache(void) {
    if (uid_cache_built) return;

    FILE* f = fopen("/etc/passwd", "r");
    if (!f) { uid_cache_built = 1; return; }

    char line[256];
    while (fgets(line, sizeof(line), f) && uid_cache_len < UID_CACHE_SIZE) {
        char* c1 = strchr(line, ':');
        if (!c1) continue;
        char* c2 = strchr(c1 + 1, ':');
        if (!c2) continue;

        uid_t uid = (uid_t) atoi(c2 + 1);

        size_t len = c1 - line;
        if (len >= sizeof(uid_cache[uid_cache_len].name))
            len = sizeof(uid_cache[uid_cache_len].name) - 1;

        uid_cache[uid_cache_len].uid = uid;
        memcpy(uid_cache[uid_cache_len].name, line, len);
        uid_cache[uid_cache_len].name[len] = '\0';
        uid_cache_len++;
    }

    fclose(f);
    uid_cache_built = 1;
}

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
    build_uid_cache();

    for (int i = 0; i < uid_cache_len; i++)
        if (uid_cache[i].uid == (uid_t)uid)
            return uid_cache[i].name;

    return NULL;
}

void refresh_processes(processes_map_t* m, unsigned long long cpu_total_delta) {
    cur_gen++;

    DIR* dir = opendir("/proc");
    if(!dir) return;

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

        int uid = read_process_uid(&p);
        if( uid >= 0) {
            char* name = resolve_user_with_uid(uid);
            if(name) snprintf(p.user, sizeof(p.user), "%s", name);
        }

        process_data_t* existing = get_process(m, pid);

        if(cur_gen == 1 || (int) cur_gen % 5 == p.pid % 5) read_pss(&p);
        else p.pss_kb = existing ? existing->pss_kb : 0;

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

