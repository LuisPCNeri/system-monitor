#ifndef MEM_H
#define MEM_H

typedef struct mem_info_t {
    long total_kb;
    long used_kb;
    long total_mib;
    long used_mib;

    long swap_total;
    long swap_used;
} mem_info_t;

/*
 * \brief Reads MemTotal and MemAvailable from /proc/meminfo
 * \param out Pointer to address to store the mem info.
 * \returns 0 on success -1 on failure.
 * */
int mem_read(mem_info_t* out);

#endif


