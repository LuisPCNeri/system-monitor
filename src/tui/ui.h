#ifndef __UI_H__
#define __UI_H__

#include "../proc/mem.h"
#include "../proc/proc.h"

/*
 *  \brief Setus up the colors and all ncurses options necessary to render the TUI. 
 * */
void tui_init();

/*
 *  \brief Renders the TUI.
 * */
void tui_render(float cpu_pct, const mem_info_t* mem, int scroll, process_data_t* processes_list, 
                size_t count, const char* search, int app_mode, int cursor, float cpu_temp, float gpu_temp, char* gpu_name,
                float uptime, float avg_load, float power_draw);

/*
 * \brief Gets the character inputed by the user.
 * \returns The character inputed by the user.
 * */
int ui_getc();

/*
 *  \brief Destroys the ncurses windows for the TUI.
 * */
void tui_destroy();

#endif
