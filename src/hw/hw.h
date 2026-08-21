#ifndef __HW__
#define __HW__

#define MAX_PATH_LEN 512

typedef struct hw_monitor_t hw_monitor_t;

/*
 *  \brief Initializes an hw_monitor_t object along with its data.
 *  \returns An hw_monitor_t pointer or NULL if there was an error.
 * */
hw_monitor_t* init_hw_monitor();

/*
 *  \brief Reads the temperature of the cpu.
 *  \param hw An initialized hw_monitor_t pointer.
 *  \returns The cpu temp in ºC or -1.0f if there was an error. 
 * */
float read_cpu_temp(hw_monitor_t* hw);

/*
 *  \brief Reads the temperature of the gpu, note that it will not work with nvidia gpus.
 *  \param hw An initialized hw_monitor_t pointer.
 *  \returns The gpu temp in ºC or -1.0f if there was an error.
 * */
float read_gpu_temp(hw_monitor_t* hw);

#endif
