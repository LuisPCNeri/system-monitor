#ifndef __UI_H__
#define __UI_H__

#include "../proc/mem.h"
#include "../proc/proc.h"

void tui_init();
void tui_render(float cpu_pct, const mem_info_t* mem, int scroll, process_data_t* processes_list, 
                size_t count, const char* search, int app_mode, int cursor);
int ui_getc();
void tui_destroy();

#endif
