#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/types.h>

typedef struct process_data_t {

    pid_t pid;
    char user[32];
    char name[256];

    char state;

    long pss_kb;
    long rss_kb;

    float cpu_pct;
    float irix_cpu_pct;

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

/*
 *  \brief Initializes an empty map struct with the given capacity or 100 if capacity was invalid.
 *  \param capacity Number of buckets in the hashmap
 *  \returns A proccesses_map_t* or NULL if there was an error.
 * */
processes_map_t* create_map(int capacity);

/*
 *  \brief Inserts a processes data into the process map.
 *  \param m The process map where to put the new data.
 *  \param data The data to insert in the map.
 * */
void insert_proc(processes_map_t* m, process_data_t data);

/*
 *  \brief Gets the process with pid from the processes map.
 *  \param m The map to search.
 *  \param pid The pid of the wanted process.
 *  \returns A pointer to the requested process' data or NULL if it was not found.
 * */
process_data_t* get_process(processes_map_t* m, pid_t pid);

/*
 *  \brief Transforms the map into a list containing all of the processes in the map.
 *  REPLACES the data in the *out parameter with newer data.
 *  \param m The map to convert.
 *  \param out A pointer to an array of process_data_t.
 *  \param max_size The max size of the out buffer.
 *  \returns The size of the list, or 0 if the size is 0 or **out is an invalid pointer.
 * */
size_t get_all_processes(processes_map_t* m, process_data_t* out, size_t max_size);

/*
 *  \brief Frees the memory taken by the map.
 *  \param m The map to free.
 * */
void free_processes_map(processes_map_t* m);

/*
 *  \brief Filters the map by removing all entries that do not conform with the condition passed as a parameter.
 *  \param map The map to filter.
 *  \param cond The condition to apply to all map entries, this condition must result in an intenger, for example p.pid > 100 
 * */
void filter_map(processes_map_t* map, FilterFunc cond);

int resize_map(processes_map_t* m);

#endif
