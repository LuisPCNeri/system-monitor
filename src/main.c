#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>

#include "proc/cpu.h"
#include "proc/mem.h"
#include "proc/proc.h"
#include "tui/ui.h"
#include "utils/hmap.h"


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

/* ------------------------------------------------------------------ */

int main(void) {
    setup_signals();
    tui_init();

    processes_map_t* map = create_map(4096);
    CpuStat cpu_a, cpu_b;
    mem_info_t mem;

    int scroll  = 0;
    int sort_by = 0;

    cpu_read(&cpu_a);
    sleep_ms(500);

    while (g_running) {

        if (g_resized) {
            g_resized = 0;
            endwin();
            refresh();
        }

        cpu_read(&cpu_b);
        unsigned long long cpu_delta = cpu_total_delta(&cpu_a, &cpu_b);
        float cpu_pct = cpu_usage(&cpu_a, &cpu_b);
        cpu_a = cpu_b;   /* shift window */

        mem_read(&mem);

        refresh_processes(map, cpu_delta);
        process_data_t* list;
        get_all_processes(map, &list);

        qsort(list, (size_t)map->size, sizeof(process_data_t),
              sort_by == 0 ? proc_cmp_cpu : proc_cmp_mem);

        int max_scroll = map->size > 0 ? map->size - 1 : 0;
        if (scroll > max_scroll) scroll = max_scroll;

        tui_render(cpu_pct, &mem, map, scroll, list);

        sleep_ms(500);

        int ch;
        while ((ch = ui_getc()) != ERR) {
            switch (ch) {
                case 'q': case 'Q':
                    g_running = 0;
                    break;
                case KEY_UP:
                    if (scroll > 0) scroll--;
                    break;
                case KEY_DOWN:
                    if (scroll < max_scroll) scroll++;
                    break;
                case KEY_PPAGE:   /* Page Up */
                    scroll -= 10;
                    if (scroll < 0) scroll = 0;
                    break;
                case KEY_NPAGE:   /* Page Down */
                    scroll += 10;
                    if (scroll > max_scroll) scroll = max_scroll;
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

    tui_destroy();
    free_processes_map(map);
    return 0;
}
