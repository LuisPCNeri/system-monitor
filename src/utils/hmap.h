#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/types.h>

typedef struct process_data_t {

    pid_t pid;
    char name[256];

    char state;

    long pss_kb;
    long rss_kb;

    float cpu_pct;

    unsigned long long utime;
    unsigned long long stime;

    unsigned long long gen;
} process_data_t;

typedef struct process_node_t process_node_t;

typedef struct processes_map_t {

    process_node_t** buckets;
    int capacity;
    size_t size;

} processes_map_t;

typedef int (*FilterFunc)(process_data_t* data);


processes_map_t* create_map(int capacity);
void insert_proc(processes_map_t* m, process_data_t data);
process_data_t* get_process(processes_map_t* m, pid_t pid);
size_t get_all_processes(processes_map_t* m, process_data_t** out);
void free_processes_map(processes_map_t* m);

void filter_map(processes_map_t* map, FilterFunc cond);

#endif
