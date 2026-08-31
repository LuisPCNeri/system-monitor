#ifndef __SYSTEM_STATS__
#define __SYSTEM_STATS__

typedef struct system_stats system_stats;

/*
 *  \brief Gets the current system uptime from /proc/uptime
 *  \returns The current system uptime in seconds or -1.0f if there was an error. 
 * */
float read_system_uptime_sec();

/*
 *  \brief Gets the system cpu avg load for the last 5 min from /proc/loadavg
 *  \returns The avg cpu load for the last 5 min or -1.0f if there was an error.
 * */
float read_system_avg_load_5();

/*
 *  \brief Inits a system_stats struct and returns a pointer to it or NULL.
 *  \returns A system_stats* or NULL.
 * */
system_stats* init_st();

/*
 *  \brief Gets the system power draw in watts.
 *  \param st An initialized system_stats struct pointer.
 *  \returns The power consumption in watts or -1 if there was an error.
 * */
float read_power_draw_watts(system_stats* st);

/*
 *  \brief Frees the memory allocated by init_st.
 *  \param st An initialized or dinamically allocated system_stats pointer.
 * */
void free_st(system_stats* st);

#endif
