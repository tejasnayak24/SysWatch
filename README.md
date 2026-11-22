# SysWatch

SysWatch is an interactive Linux terminal monitoring dashboard and process manager written in C. The default interface is a dark ncurses dashboard inspired by tools such as htop and btop, while the original command-oriented interface remains available for scripts and automation.

## Features

- ncurses dashboard with consistent colours, section headers, CPU and memory progress bars, and a process table.
- Background pthread collector for CPU, memory, and `/proc` process data so the UI remains responsive.
- Keyboard navigation: `↑`/`↓` select a process, `k` confirms `SIGTERM`, `K` confirms the explicit `SIGKILL` fallback, and `q`/`Esc` exits.
- Terminal resize handling, small-terminal guidance, invalid input handling, and clean SIGINT/thread shutdown.
- Process listing/search with PID, process name, and resident memory (RSS).
- Existing process creation using `fork()`, `execvp()`, and `waitpid()`.
- Existing process management with `SIGTERM` by default and `SIGKILL` only with `--force`.

## Architecture

- `cpu_monitor`: reads `/proc/stat` and calculates CPU utilization.
- `memory_monitor`: reads `/proc/meminfo`, exposing reusable memory metrics and preserving the original printable API.
- `process_monitor`: scans numeric `/proc/<pid>` directories and tolerates processes disappearing during a scan.
- `dashboard`: collects metrics in a mutex-protected background pthread and provides copied snapshots to the UI.
- `dashboard_ui`: owns ncurses rendering, keyboard input, selection, confirmation, resize behavior, and shutdown.
- `process_manager` and `process_launcher`: retain signal and fork/exec/wait functionality.
- `main`: selects the dashboard or non-interactive command mode.

## Dependencies

Linux, a C compiler, GNU Make, POSIX pthreads, and ncurses development files. On Debian/Ubuntu:

```sh
sudo apt install build-essential libncurses-dev pkg-config
```

No third-party runtime libraries beyond ncurses are required.

## Build and run

```sh
make build       # compile syswatch
make dashboard   # launch the interactive dashboard
make test        # run smoke and CLI regression tests
make clean       # remove generated objects and executable
```

Running `./syswatch` or `make dashboard` opens the interactive dashboard. The interface refreshes automatically while the collector reads current metrics in the background.

## Non-interactive CLI

```text
./syswatch snapshot
./syswatch list [name]
./syswatch search <name>
./syswatch terminate <pid> [--force]
./syswatch run <command> [args...]
./syswatch watch [seconds]
./syswatch --help
```

The `snapshot`, `list`, `search`, `terminate`, `run`, and `watch` commands are suitable for terminals, scripts, and environments without an interactive terminal.

## WSL limitations

WSL provides a Linux-compatible `/proc`, so the dashboard normally works. Its process list represents the WSL environment rather than the complete Windows host, and Windows processes cannot be managed through Linux signals. Resource values can differ from Windows Task Manager. Restricted containers or WSL configurations may deny access to some `/proc/<pid>` files. An interactive terminal is required for the ncurses dashboard; use the non-interactive commands from scripts or redirected sessions.

## Testing

`make test` verifies help output, process listing/search, the `fork`/`execvp`/`waitpid` path, snapshot output, safe termination of a temporary test process, and invalid-argument handling. The dashboard entry point was also exercised through a pseudo-terminal/input path; visual appearance should be checked in the user’s terminal emulator and resized manually.
