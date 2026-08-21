#include <bits/time.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>

#include "proc/cpu.h"
#include "proc/mem.h"
#include "proc/proc.h"
#include "tui/ui.h"
#include "utils/hmap.h"
#include "hw/hw.h"
#include "hw/system_stats.h"

static volatile sig_atomic_t g_running = 1;
static volatile sig_atomic_t g_resized = 0;

static void on_sigint(int sig)   { (void)sig; g_running = 0; }
static void on_sigwinch(int sig) { (void)sig; g_resized = 1; }

static void setup_signals(void) {
    struct sigaction sa = {0};
    sa.sa_flags = SA_RESTART;    sa.sa_handler = on_sigint;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = on_sigwinch;
    sigaction(SIGWINCH, &sa, NULL);
}

/* ------------------------------------------------------------------ */

static void sleep_ms(long ms) {
    struct timespec ts = {
        .tv_sec  = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L
    };
    nanosleep(&ts, NULL);
}

typedef enum {MODE_NORMAL = 0, MODE_SEARCH = 1} app_mode;

int main(void) {
    setup_signals();
    tui_init();

    hw_monitor_t* hw = init_hw_monitor();
    if(!hw) return -1;

    float cpu_temp = read_cpu_temp(hw);
    float gpu_temp = read_gpu_temp(hw);

    processes_map_t* map = create_map(4096);
    CpuStat cpu_a, cpu_b;
    mem_info_t mem;


    float uptime = 0.0f, load_avg = 0.0f;

    int cursor  = 0;
    int scroll  = 0;
    int sort_by = 0;

    app_mode mode        = MODE_NORMAL;
    char search_buf[256] = {0};
    int search_len       = 0;

    cpu_read(&cpu_a);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    t0.tv_sec -= 1;

    int is_starting = 1;

    int max_scroll = 0;
    process_data_t* list = NULL;
    size_t count = 0;
    float cpu_pct = 0.0f;

    int rows = getmaxy(stdscr);
    int visible_rows = rows - 9;

    while (g_running) {
        if (g_resized) {
            g_resized = 0;
            endwin();
            refresh();
        }


        clock_gettime(CLOCK_MONOTONIC, &t1);
        long elapsed = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1000000;

        int need_refilter = 0;

        if(is_starting || elapsed >= 1500) {
            is_starting = 0;
            t0 = t1;

            cpu_temp = read_cpu_temp(hw);
            gpu_temp = read_gpu_temp(hw);

            uptime   = read_system_uptime_sec();
            load_avg = read_system_avg_load_5();

            cpu_read(&cpu_b);
            unsigned long long cpu_delta = cpu_total_delta(&cpu_a, &cpu_b);
            cpu_pct = cpu_usage(&cpu_a, &cpu_b);
            cpu_a = cpu_b;   /* shift window */

            mem_read(&mem);

            refresh_processes(map, cpu_delta);
            need_refilter = 1;
        }

        int ch;
        while ((ch = ui_getc()) != ERR) {

            if(mode == MODE_SEARCH) {
 
                if(ch == 27) {
                    search_buf[0] = '\0';
                    search_len = 0;
                    mode = MODE_NORMAL;
                    scroll = 0;
                    need_refilter = 1;
                }
                else if(ch == '\n') {
                    mode = MODE_NORMAL;
                    scroll = 0;
                }
                else if(ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                    if(search_len > 0) search_buf[--search_len] = '\0';
                    need_refilter = 1;
                }
                else if (ch >= 32 && ch <= 127 && search_len < 255) {
                    search_buf[search_len++] = (char) ch;
                    search_buf[search_len]   = '\0';
                    need_refilter = 1;
                }
            }
            else {
                switch (ch) {
                case 'q': case 'Q':
                    g_running = 0;
                    break;
                case 's': case 'S': case 'f': case 'F':
                    mode = MODE_SEARCH;
                    break;
                case 'k': case 'K':
                    if(cursor >= 0 && cursor < (int) count) {
                        process_data_t* p = &list[cursor];
                        kill_process(p->pid);
                    }
                    break;
                    case 't': case 'T': case KEY_DC:
                    if(cursor >= 0 && cursor < (int) count) {
                        process_data_t* p = &list[cursor];
                        kill(p->pid, SIGTERM);
                    }
                    break;
                case KEY_UP:
                    if (cursor > 0) cursor--;
                    if (cursor < scroll) scroll = cursor;
                    break;
                case KEY_DOWN:
                    if (cursor < (int) count - 1) cursor++;
                    if (cursor >= scroll + visible_rows) scroll = cursor - visible_rows + 1;
                    break;
                case KEY_PPAGE:
                    if (cursor > 10) cursor -= 10;
                    else cursor = 0;
                    if (cursor < scroll) scroll = cursor;

                    break;
                case KEY_NPAGE:
                    if (cursor < (int) count - 10) cursor += 10;
                    else cursor = (int) count - 1;
                    if (cursor >= scroll + visible_rows - 10) scroll = cursor - visible_rows + 1;

                    break;
                case 'c': case 'C':
                    sort_by = 0;
                    break;
                case 'm': case 'M':
                    sort_by = 1;
                    break;
                }
            }
        }


        if(need_refilter) {
            free(list);
            count = get_all_processes(map, &list);

            if(search_len > 0) {
                size_t out = 0;
                for(size_t i = 0; i < count; i++) {
                    if(strcasestr(list[i].name, search_buf)) {
                        list[out++] = list[i];
                    }
                }

                count = out;
            }

            qsort(list, count, sizeof(process_data_t),
                  sort_by == 0 ? proc_cmp_cpu : proc_cmp_mem);

            max_scroll = map->size > 0 ? map->size - 1 : 0;
            if (scroll > max_scroll) scroll = max_scroll;
        }

        if(list) tui_render(cpu_pct, &mem, scroll, list, count, search_buf, mode, cursor, cpu_temp, gpu_temp,
                            uptime, load_avg);
        sleep_ms(16);
    }

    if(list) free(list);
    tui_destroy();
    free_processes_map(map);
    return 0;
}
