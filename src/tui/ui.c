#include <ncurses.h>
#include <stddef.h>
#include <stdio.h>

#include "../proc/mem.h"
#include "../utils/hmap.h"

#define CP_NORMAL 1
#define CP_WARN   2
#define CP_CRIT   3
#define CP_HEADER 4
#define CP_DIM    5

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

        init_pair(CP_NORMAL, COLOR_GREEN,  -1);
        init_pair(CP_WARN,   COLOR_YELLOW, -1);
        init_pair(CP_CRIT,   COLOR_RED,    -1);
        init_pair(CP_HEADER, COLOR_CYAN,   -1);
        init_pair(CP_DIM,    COLOR_WHITE,  -1);
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

    for(int i = 0; i < filled; i++) mvaddch(y, x + 1 + i, ' ');
    mvaddch(y, x + w - 1, ']');

    if (suffix) mvaddstr(y, x + w + 1, suffix);
}

void tui_render(float cpu_pct, const mem_info_t* mem, processes_map_t* map, int scroll) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    erase();

    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvprintw(0, 0, " sysmon");
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);


    char cpu_suffix[32];
    snprintf(cpu_suffix, sizeof(cpu_suffix), "%.1f%%", cpu_pct);

    int bar_w = (cols / 2) - 14;
    if (bar_w < 10) bar_w = 10;

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(1, 1, "CPU ");
    attroff(COLOR_PAIR(CP_DIM));
    draw_bar(1, 5, bar_w, cpu_pct, cpu_suffix);


    float ram_pct = mem->total_mib > 0 ? 100.0f * (float) mem->used_mib / (float) mem->total_mib : 0.0f;
    char ram_suffix[64];
    snprintf(ram_suffix, sizeof(ram_suffix), "%ld / %ld MiB", mem->used_mib, mem->total_mib);

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(2, 1, "RAM ");
    attroff(COLOR_PAIR(CP_DIM));
    draw_bar(2, 5, bar_w, ram_pct, ram_suffix);


    attron(A_BOLD | COLOR_PAIR(CP_DIM));
    mvprintw(4, 0, " %7s %-20s %7s %10s %s", "PID", "NAME", "CPU %", "MEM (MiB)", "S");
    attroff(A_BOLD | COLOR_PAIR(CP_DIM));
    mvhline(5, 0, ACS_HLINE, cols);


    int vis = rows - 7;
    process_data_t* processes_list;
    if(!get_all_processes(map, &processes_list)) {
        return;
    }

    for(int i = 0; i < vis && (scroll + i) < map->size ; i++) {

        const process_data_t* p = &processes_list[scroll + i];
        float rss_mib = (float) p->rss_kb / 1024.0f;
        int row = 6 + i;

        int cpu_pair = CP_NORMAL;
        if      (p->cpu_pct > 50.0f) cpu_pair = CP_CRIT;
        else if (p->cpu_pct > 20.0f) cpu_pair = CP_WARN;

        mvprintw(row, 0, " %7d %-20.20s  ", p->pid, p->name);

        attron(COLOR_PAIR(cpu_pair));
        printw("%6.1f%%", p->cpu_pct);
        attroff(COLOR_PAIR(cpu_pair));

        printw(" %9.1f %c", rss_mib, p->state);
    }


    attron(COLOR_PAIR(CP_DIM));
    mvprintw(rows - 1, 0,
         " [q]uit  [↑/↓] scroll  [c] sort CPU  [m] sort RAM");
    attroff(COLOR_PAIR(CP_DIM));

    refresh();
}

int ui_getc() {
    return getch();
}

