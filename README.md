# sysmon

A lightweight terminal system monitor for Linux, written in C with ncurses.

## Features

- Real-time CPU and RAM usage bars with color-coded load levels
- Live process table (PID, name, CPU %, RSS, PSS, state)
- Sort processes by CPU (`c`) or memory (`m`)
- Search/filter processes by name (`s` / `f`)
- Scrollable process list with a highlighted selection
- Terminate the selected process (`k`)
- Colors indicate load: green (normal), yellow (warning), red (critical)
- Data sourced directly from `/proc`

## Requirements

- Linux with a `/proc` filesystem
- gcc
- A local ncurses build for the static binary (see [Build](#build)), or the system ncurses dev library (e.g. `libncurses-dev` on Debian/Ubuntu) for `make shared`

## Install

Prebuilt binaries are available in the [GitHub Releases](https://github.com/LuisPCNeri/system-monitor/releases).

## Build

The default `make` target produces a self-contained static binary, linked against a locally built ncurses. The Makefile expects that build at `$(HOME)/built_pckgs/ncurses-6.4` — adjust `NCURSES_DIR` at the top of the Makefile if yours lives elsewhere.

    make

For a smaller binary that links against the system's ncurses shared library instead:

    make shared

## Usage

    ./sysmon

## Controls

| Key | Action |
|-----|--------|
| q | Quit |
| UP / DOWN | Move selection / scroll process list |
| PgUp / PgDn | Scroll by 10 lines |
| s / f | Enter search mode |
| Esc | Cancel search |
| Enter | Confirm search |
| c | Sort by CPU % |
| m | Sort by RAM usage |
| k | Terminate selected process |

## Project layout

- `src/main.c` — entry point, main loop, input handling
- `src/proc/` — `/proc` data readers (`cpu.c`, `mem.c`, `proc.c`)
- `src/tui/` — ncurses interface (`ui.c`)
- `src/utils/` — hash map used to track processes (`hmap.c`)

## License

MIT — see `LICENSE`.
