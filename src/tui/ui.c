#include <ncurses.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "ui.h"
#include "../hw/hw.h"
#include "../proc/mem.h"
#include "../utils/hmap.h"

#define CP_NORMAL   1
#define CP_WARN     2
#define CP_CRIT     3
#define CP_HEADER   4
#define CP_DIM      5
#define CP_SELECTED 6

void tui_init() {
    initscr();
    /* to prevent the switch from search to normal mode using the escape key taking so long*/
    set_escdelay(25);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    if( has_colors() ) {
        start_color();
        use_default_colors();

        init_pair(CP_NORMAL,   COLOR_GREEN,  -1);
        init_pair(CP_WARN,     COLOR_YELLOW, -1);
        init_pair(CP_CRIT,     COLOR_RED,    -1);
        init_pair(CP_HEADER,   COLOR_CYAN,   -1);
        init_pair(CP_DIM,      COLOR_WHITE,  -1);
        init_pair(CP_SELECTED, COLOR_BLACK,  COLOR_CYAN);
    }
}

void tui_destroy() {
    endwin();
}

static void draw_bar(int y, int x, int w, float pct, const char* suffix) {

    int inner = w - 2;
    if (inner < 1) inner = 1;

    int filled = (int) (pct / 100.0f * (float) inner);
    if (filled > inner) filled = inner;
    if (filled < 0)     filled = 0;

    int pair = CP_NORMAL;
    if (pct > 85.0f) pair = CP_CRIT;
    else if (pct > 50.0f) pair = CP_WARN;

    char bar[w];
    mvaddch(y, x, '[');

    attron(COLOR_PAIR(pair));
    memset(bar, '|', filled);
    memset(bar + filled, ' ', inner - filled);
    bar[inner] = '\0';
    mvaddstr(y, x + 1, bar);
    attroff(COLOR_PAIR(pair));

    mvaddch(y, x + w - 1, ']');

    if (suffix) mvaddstr(y, x + w + 1, suffix);
}

void tui_render(float cpu_pct, const mem_info_t* mem, int scroll, process_data_t* processes_list, 
                size_t count, const char* search, int app_mode, int cursor, float cpu_temp, float gpu_temp, char* gpu_name,
                float uptime, float avg_load, float power_draw, mount_info_t* mounts, short mount_amt, int mount_scroll) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    erase();

    int bar_w = (cols / 2) - 30;
    if (bar_w < 10) bar_w = 10;

    /* header bar with app name, system uptime and last 5 min load avg */

    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvprintw(0, 0, " sysmon");
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    attron(COLOR_PAIR(CP_DIM)  | A_DIM);
    if(power_draw < 0) power_draw = 0;

    if(uptime >= 3600) {
        float mins = (int) uptime % 3600;
        mins = mins / 60;

        uptime /= 3600;
        mvprintw(0, 8, "uptime: %.0f hr, %.0f min | 5min cpu avg load: %.2f%% | power consumption: %.2fW", uptime, mins, avg_load, power_draw);
    }
    else if(uptime >= 60) {
        uptime /= 60;
        mvprintw(0, 8, "uptime: %.0f min | 5min cpu avg load: %.2f%% | power consumption: %.2fW", uptime, avg_load, power_draw);
    }
    attroff(COLOR_PAIR(CP_DIM) | A_DIM);


    /* cpu usage bar, total cpu usage %, cpu temp */

    char cpu_suffix[32];
    snprintf(cpu_suffix, sizeof(cpu_suffix), "%.1f%%", cpu_pct);

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(2, 1, "CPU ");
    attroff(COLOR_PAIR(CP_DIM));
    draw_bar(2, 6, bar_w, cpu_pct, cpu_suffix);

    if (cpu_temp >= 0.0f) {
        int temp_pair = CP_NORMAL;
        if      (cpu_temp > 90.0f) temp_pair = CP_CRIT;
        else if (cpu_temp > 70.0f) temp_pair = CP_WARN;

        int temp_x = 6 + bar_w + 1 + (int)strlen(cpu_suffix) + 1;
        attron(COLOR_PAIR(temp_pair));
        mvprintw(2, temp_x, "%.1f°C", cpu_temp);
        attroff(COLOR_PAIR(temp_pair));
    }


    /* ram usage bar, used ram out of total */

    float ram_pct = mem->total_mib > 0 ? 100.0f * (float) mem->used_mib / (float) mem->total_mib : 0.0f;
    char ram_suffix[64];
    snprintf(ram_suffix, sizeof(ram_suffix), "%.2f / %.2f GiB", (float) mem->used_mib / 1024.0f, (float) mem->total_mib / 1024.0f);

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(3, 1, "RAM ");
    attroff(COLOR_PAIR(CP_DIM));
    draw_bar(3, 6, bar_w, ram_pct, ram_suffix);


    /* swap usage bar, used swap out of total */

    float swap_pct = mem->swap_total > 0 ? 100.0f * (float) mem->swap_used / (float) mem->swap_total : 0.0f;
    char swap_suffix[64];
    snprintf(swap_suffix, sizeof(swap_suffix), "%.2f / %.2f GiB", (float) mem->swap_used / 1024.0f, (float) mem->swap_total / 1024.0f);

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(4, 1, "SWAP ");
    attroff(COLOR_PAIR(CP_DIM));

    draw_bar(4, 6, bar_w, swap_pct, swap_suffix);


    /* if a gpu_temp was recorded gpu temp here */
     if (gpu_temp >= 0.0f) {
        int gpu_pair = CP_NORMAL;
        if      (gpu_temp >= 85.0f) gpu_pair = CP_CRIT;
        else if (gpu_temp >= 80.0f) gpu_pair = CP_WARN;

        mvprintw(5, 1, "%s", gpu_name);

        attron(COLOR_PAIR(gpu_pair));
        mvprintw(5, strlen(gpu_name) + 2, "%.1f\xc2\xb0""C", gpu_temp);
        attroff(COLOR_PAIR(gpu_pair));
    }


    /* mount bars on right half, rows 1-3 */
    {
        int right_x     = cols / 2;
        int right_w     = cols - right_x;
        int label_w     = 12;
        int suffix_res  = 22;  // "1023.9/1023.9GB\0"
        int mount_bar_w = right_w - label_w - 3 - suffix_res;
        if (mount_bar_w < 8) mount_bar_w = 8;

        int page_size   = 3;
        int start       = mount_scroll * page_size;

        for (int i = 0; i < page_size && start + i < mount_amt; i++) {
            mount_info_t* m = &mounts[start + i];
            int row = 2 + i;

            char suffix[32];
            double total_gb = m->total / (1024.0 * 1024.0 * 1024.0);
            double used_gb  = m->used  / (1024.0 * 1024.0 * 1024.0);
            if (total_gb >= 1.0)
                snprintf(suffix, sizeof(suffix), "%.2f / %.2f GiB", used_gb, total_gb);
            else
                snprintf(suffix, sizeof(suffix), "%.2f / %.2f MiB",
                         m->used / (1024.0 * 1024.0), m->total / (1024.0 * 1024.0));

            float pct = m->total > 0
                ? 100.0f * (float)m->used / (float)m->total
                : 0.0f;

            attron(COLOR_PAIR(CP_DIM));
            mvprintw(row, right_x, "%-*.*s", label_w, label_w, m->mount_point);
            attroff(COLOR_PAIR(CP_DIM));

            draw_bar(row, right_x + label_w + 1, mount_bar_w, pct, suffix);
        }
    }

    /* data headers and line */

    attron(A_BOLD | COLOR_PAIR(CP_DIM));
    mvprintw(7, 0, " %7s %-20s %7s %9s %11s %10s %8s %-20s", "PID", "NAME", "CPU %", "CORE %", "RSS", "PSS", "S", "USER");
    attroff(A_BOLD | COLOR_PAIR(CP_DIM));
    mvhline(8, 0, ACS_HLINE, cols);


    /* process data list */

    int vis = rows - 10;

    for(int i = 0; i < vis && (scroll + i) < (int) count ; i++) {

        const process_data_t* p = &processes_list[scroll + i];
        if(p->pid == 0) continue;

        float rss = (float) p->rss_kb / 1024.0f;
        float pss = (float) p->pss_kb / 1024.0f;
        int row = 9 + i;

        int is_selected = (scroll + i) == cursor;
        if (is_selected) {
            attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
            mvhline(row, 0, ' ', cols);
        }

        int cpu_pair  = CP_NORMAL;
        int proc_attr = A_NORMAL;
        if (!is_selected) {
            if      (p->cpu_pct > 50.0f) cpu_pair = CP_CRIT;
            else if (p->cpu_pct > 20.0f) cpu_pair = CP_WARN;
            else if (p->cpu_pct == 0.0f) {
                cpu_pair = CP_DIM;
                proc_attr = A_DIM;
            }
        }

        int core_pair = CP_NORMAL;
        if(!is_selected) {
            if     (p->irix_cpu_pct > 70.0f) core_pair = CP_CRIT;
            else if(p->irix_cpu_pct > 50.0f) core_pair = CP_WARN;
            else if(p->irix_cpu_pct == 0.0f) core_pair = CP_DIM;
        }


        attron(proc_attr);
        mvprintw(row, 0, " %7d %-20.20s ", p->pid, p->name);

        if (!is_selected) attron(COLOR_PAIR(cpu_pair));
        printw("%6.2f%%", p->cpu_pct);
        if (!is_selected) attroff(COLOR_PAIR(cpu_pair));

        if (!is_selected) attron(COLOR_PAIR(core_pair));
        printw("%8.1f%%", p->irix_cpu_pct);
        if (!is_selected) attroff(COLOR_PAIR(core_pair));

        attroff(proc_attr);

        char unit_rss[8] = "MiB";
        char unit_pss[8] = "MiB";

        if(rss <= 0) {
            rss = p->rss_kb;
            sprintf(unit_rss, "kB");
        }
        if(pss <= 0) {
            pss = p->pss_kb;
            sprintf(unit_pss, "kB");
        }

        char rss_block[24], pss_block[24];
        snprintf(rss_block, sizeof rss_block, "%.1f %s", rss, unit_rss);
        snprintf(pss_block, sizeof pss_block, "%.1f %s", pss, unit_pss);

        printw(" %12s %11s %7c", rss_block, pss_block, p->state);
        printw(" %-20.20s", p->user);

        if (is_selected) attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);
    }


    /* footer */
    const char* footer = " [q]uit  [UP/DOWN] scroll  [c]pu sort  [m]em sort  [k]ill  [t]erm  [</>] disks";

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(rows - 1, 0, footer);
    attroff(COLOR_PAIR(CP_DIM));

    if (search && (search[0] != '\0' || app_mode == 1)) {
        char search_str[270];
        snprintf(search_str, sizeof(search_str), "Search: %s |", search);

        int search_x = cols - (int)strlen(search_str) - 1;
        if (search_x > (int)strlen(footer)) {  // don't overlap footer text
            attron(COLOR_PAIR(CP_WARN));
            mvaddstr(rows - 1, search_x, search_str);
            attroff(COLOR_PAIR(CP_WARN));
        }
    }
    else {
        attron(COLOR_PAIR(CP_DIM));
        mvaddstr(rows - 1, cols - (int) sizeof("[s]earch") - 1, "[s]earch");
        attroff(COLOR_PAIR(CP_DIM));
    }

    wnoutrefresh(stdscr);
    doupdate();
}

int ui_getc() {
    return getch();
}

