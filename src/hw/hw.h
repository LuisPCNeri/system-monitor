#ifndef __HW__
#define __HW__

#define MAX_PATH_LEN 512

typedef struct hw_monitor_t hw_monitor_t;

hw_monitor_t* init_hw_monitor();
float read_cpu_temp(hw_monitor_t* hw);
float read_gpu_temp(hw_monitor_t* hw);

#endif
