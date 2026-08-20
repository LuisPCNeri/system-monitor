#include <ncurses.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "ui.h"
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
    else if (pct > 60.0f) pair = CP_WARN;

    mvaddch(y, x, '[');
    attron(COLOR_PAIR(pair));

    for(int i = 0; i < filled; i++) mvaddch(y, x + 1 + i, '|');
    attroff(COLOR_PAIR(pair));

    for(int i = filled; i < inner; i++) mvaddch(y, x + 1 + i, ' ');
    mvaddch(y, x + w - 1, ']');

    if (suffix) mvaddstr(y, x + w + 1, suffix);
}

void tui_render(float cpu_pct, const mem_info_t* mem, int scroll, process_data_t* processes_list, 
                size_t count, const char* search, int app_mode, int cursor, float cpu_temp, float gpu_temp) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    erase();

    int bar_w = (cols / 2) - 14;
    if (bar_w < 10) bar_w = 10;

    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvprintw(0, 0, " sysmon");
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    char cpu_suffix[32];

    snprintf(cpu_suffix, sizeof(cpu_suffix), "%.1f%%", cpu_pct);

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(1, 1, "CPU ");
    attroff(COLOR_PAIR(CP_DIM));
    draw_bar(1, 5, bar_w, cpu_pct, cpu_suffix);

    if (cpu_temp >= 0.0f) {
        int temp_pair = CP_NORMAL;
        if      (cpu_temp > 90.0f) temp_pair = CP_CRIT;
        else if (cpu_temp > 70.0f) temp_pair = CP_WARN;

        int temp_x = 5 + bar_w + 1 + (int)strlen(cpu_suffix) + 1;
        attron(COLOR_PAIR(temp_pair));
        mvprintw(1, temp_x, "%.1f°C", cpu_temp);
        attroff(COLOR_PAIR(temp_pair));
    }


    float ram_pct = mem->total_mib > 0 ? 100.0f * (float) mem->used_mib / (float) mem->total_mib : 0.0f;
    char ram_suffix[64];
    snprintf(ram_suffix, sizeof(ram_suffix), "%.2f / %.2f GiB", (float) mem->used_mib / 1024.0f, (float) mem->total_mib / 1024.0f);

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(2, 1, "RAM ");
    attroff(COLOR_PAIR(CP_DIM));
    draw_bar(2, 5, bar_w, ram_pct, ram_suffix);

    if(gpu_temp >= 0.0f) {

        char gpu_str[64];
        snprintf(gpu_str, sizeof(gpu_str), "GPU %.1fºC", gpu_temp);

        attron(COLOR_PAIR(CP_DIM));
        mvaddstr(3, 1, gpu_str);
        attroff(COLOR_PAIR(CP_DIM));
    }


    if( (search && search[0] != '\0') || (search[0] == '\0' && app_mode == 1)) {
        attron(COLOR_PAIR(CP_WARN));
        mvprintw(3, 1, "Search: %s |", search);
        attroff(COLOR_PAIR(CP_WARN));
    }
    else {
        attron(COLOR_PAIR(CP_DIM));
        mvprintw(3, 1, "[s]earch");
        attroff(COLOR_PAIR(CP_DIM));
    }


    attron(A_BOLD | COLOR_PAIR(CP_DIM));
    mvprintw(4, 0, " %7s %7s  %-20s %7s %9s %11s %10s %8s", "PID", "USER", "NAME", "CPU %", "CORE %", "RSS", "PSS", "S");
    attroff(A_BOLD | COLOR_PAIR(CP_DIM));
    mvhline(5, 0, ACS_HLINE, cols);


    int vis = rows - 7;

    for(int i = 0; i < vis && (scroll + i) < (int) count ; i++) {

        const process_data_t* p = &processes_list[scroll + i];
        if(p->pid == 0) continue;

        float rss = (float) p->rss_kb / 1024.0f;
        float pss = (float) p->pss_kb / 1024.0f;
        int row = 6 + i;

        int is_selected = (scroll + i) == cursor;
        if (is_selected) {
            attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
            mvhline(row, 0, ' ', cols);
        }

        int cpu_pair = CP_NORMAL;
        int cpu_attr = A_NORMAL;
        if (!is_selected) {
            if      (p->cpu_pct > 50.0f) cpu_pair = CP_CRIT;
            else if (p->cpu_pct > 20.0f) cpu_pair = CP_WARN;
            else if (p->cpu_pct == 0.0f) {
                cpu_pair = CP_DIM;
                cpu_attr = A_DIM;
            }
        }

        int core_pair = CP_NORMAL;
        int core_attr = A_NORMAL;
        if(!is_selected) {
            if     (p->irix_cpu_pct > 70.0f) core_pair = CP_CRIT;
            else if(p->irix_cpu_pct > 50.0f) core_pair = CP_WARN;
            else if(p->irix_cpu_pct == 0.0f) {
                core_pair = CP_DIM;
                core_attr = A_DIM;
            }
        }

        mvprintw(row, 0, " %7d %7s  %-20.20s ", p->pid, p->user, p->name);

        if (!is_selected) attron(COLOR_PAIR(cpu_pair)  | cpu_attr);
        printw("%5.2f%%", p->cpu_pct);
        if (!is_selected) attroff(COLOR_PAIR(cpu_pair) | cpu_attr);

        if (!is_selected) attron(COLOR_PAIR(core_pair)  | core_attr);
        printw("%8.1f%%", p->irix_cpu_pct);
        if (!is_selected) attroff(COLOR_PAIR(core_pair) | core_attr);

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

        if (is_selected) attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);
    }


    attron(COLOR_PAIR(CP_DIM));
    mvprintw(rows - 1, 0,
         " [q]uit  [UP/DOWN] scroll  [c] sort CPU  [m] sort RAM  [k]ill forcefully [t]erminate cleanly");
    attroff(COLOR_PAIR(CP_DIM));

    refresh();
}

int ui_getc() {
    return getch();
}

