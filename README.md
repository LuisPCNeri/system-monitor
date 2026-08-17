# sysmon

A lightweight terminal system monitor for Linux, written in C with ncurses.

## Features

- Real-time CPU and RAM usage bars with color-coded load levels
- Live process table (PID, name, CPU %, RSS, PSS, state)
- Sort processes by CPU (`c`) or memory (`m`)
- Search/filter processes by name (`s` / `f`)
- Scrollable process list
- Colors indicate load: green (normal), yellow (warning), red (critical)
- Data sourced directly from `/proc`

## Requirements

- Linux with a `/proc` filesystem
- gcc
- ncurses dev library (e.g. `libncurses-dev` on Debian/Ubuntu)

## Build

    make

## Usage

    ./sysmon

## Controls

| Key | Action |
|-----|--------|
| q | Quit |
| ↑ / ↓ | Scroll process list |
| PgUp / PgDn | Scroll by 10 lines |
| s / f | Enter search mode |
| Esc | Cancel search |
| Enter | Confirm search |
| c | Sort by CPU % |
| m | Sort by RAM usage |

## Project layout

- `src/main.c` — entry point, main loop, input handling
- `src/proc/` — `/proc` data readers (`cpu.c`, `mem.c`, `proc.c`)
- `src/tui/` — ncurses interface (`ui.c`)
- `src/utils/` — hash map used to track processes (`hmap.c`)

## License

MIT — see `LICENSE`.
