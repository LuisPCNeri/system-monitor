# sysmon

A lightweight terminal system monitor for Linux, written in C with ncurses.

## Features

- Real-time CPU, RAM and swap usage bars with color-coded load levels
- System uptime, 5-minute average CPU load and power draw (watts) in the header
- CPU temperature display, plus GPU model name and temperature when available (AMD/Intel via hwmon, NVIDIA via `nvidia-smi`)
- Per-filesystem disk usage bars, paged with `<` / `>`
- Live process table (PID, name, CPU %, per-core CPU %, RSS, PSS, state, user)
- Sort processes by CPU (`c`) or memory (`m`)
- Search/filter processes by name (`s` / `f`)
- Scrollable process list with a highlighted selection
- Kill the selected process forcefully (`k`, SIGKILL) or terminate it cleanly (`t` / `Del`, SIGTERM)
- Idle processes (0% CPU) are dimmed to reduce visual fatigue
- Colors indicate load: green (normal), yellow (warning), red (critical)
- Data sourced directly from `/proc` and `/sys`

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
| < / > | Page through disk usage bars |
| k | Kill selected process (SIGKILL) |
| t / Del | Terminate selected process (SIGTERM) |

## Project layout

- `src/main.c` — entry point, main loop, input handling
- `src/proc/` — `/proc` data readers (`cpu.c`, `mem.c`, `proc.c`)
- `src/hw/` — hardware monitoring and system stats (`hw.c`, `system_stats.c`): CPU/GPU temperatures, mounts, uptime, load average, power draw
- `src/tui/` — ncurses interface (`ui.c`)
- `src/utils/` — hash map used to track processes (`hmap.c`)

## License

MIT — see `LICENSE`.
