#include "hmap.h"

#include <sys/types.h>
#include <stdlib.h>

#define FALLBACK_CAPACITY 100

typedef struct process_node_t {

    process_data_t data;
    struct process_node_t* next;

} process_node_t;

static unsigned int hash_pid (pid_t pid, int capacity) {
    /* Thomas Wang's integer hash function*/

    unsigned int hash = (unsigned int)pid;
    hash = (hash ^ 61) ^ (hash >> 16);
    hash = hash + (hash << 3);
    hash = hash ^ (hash >> 4);
    hash = hash * 0x27d4eb2d;
    hash = hash ^ (hash >> 15);
    return hash % capacity;
}

processes_map_t* create_map(int capacity) {
    if( capacity <= 0 ) capacity = FALLBACK_CAPACITY;

    processes_map_t* map = (processes_map_t*) calloc(1, sizeof(processes_map_t));
    if (!map) return NULL;

    map->size = 0;
    map->capacity = FALLBACK_CAPACITY;
    map->buckets = calloc(capacity, sizeof(process_node_t*));
    if(!map->buckets) return NULL;

    return map;
}

void insert_proc(processes_map_t* m, process_data_t data) {
    unsigned int idx = hash_pid(data.pid, m->capacity);
    process_node_t* current = m->buckets[idx];

    while(current) {
        if(current->data.pid == data.pid) {
            current->data = data;
            return;
        }

        current = current->next;
    }

    process_node_t* new_node = (process_node_t*) malloc(sizeof(process_node_t));
    if(!new_node) return;

    new_node->data = data;
    new_node->next = m->buckets[idx];
    m->buckets[idx] = new_node;

    m->size++;
}

process_data_t* get_process(processes_map_t* m, pid_t pid) {
    unsigned int idx = hash_pid(pid, m->capacity);
    process_node_t* cur = m->buckets[idx];

    while(cur) {
        if(cur->data.pid == pid) {
            return &cur->data;
        }

        cur = cur->next;
    }

    return NULL;
}

size_t get_all_processes(processes_map_t* m, process_data_t** out) {
    if(m->size == 0) {
        *out = NULL;
        return 0;
    }

    *out = (process_data_t*) malloc(m->size * sizeof(process_data_t));
    if(!out) return 0;

    size_t idx = 0;
    for(int i = 0; i < m->capacity; i++) {
        process_node_t* cur = m->buckets[i];
        while(cur) {
            (*out)[idx++] = cur->data;
            cur = cur->next;
        }
    }

    return m->size;
}

void filter_map(processes_map_t* map, FilterFunc cond) {
    if(!map || !cond) return;

    for(int i = 0; i < map->capacity; i++) {

        process_node_t** cur = &map->buckets[i];
        while(*cur) {

            process_node_t* entry = *cur;
            if(!cond(&entry->data)) {

                *cur = entry->next;
                free(entry);
                map->size--;
            }
            else {
                *cur = entry->next;
            }
        }
    }
}


void free_processes_map(processes_map_t* m) {

    for(int i = 0; i < m->capacity; i++) {
        process_node_t* cur = m->buckets[i];
        while(cur) {
            process_node_t* next = cur->next;
            free(cur);

            cur = next;
        }
    }

    free(m->buckets);
    free(m);
}
