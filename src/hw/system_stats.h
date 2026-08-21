#ifndef __SYSTEM_STATS__
#define __SYSTEM_STATS__

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

#endif
