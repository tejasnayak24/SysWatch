# SysWatch

SysWatch is a Linux command-line system monitor and process manager written in C. It uses Linux `/proc` data and POSIX system calls to expose CPU and memory information, inspect processes, manage process lifetime, launch commands, and run periodic monitoring in a pthread.

## Features

- CPU utilization from `/proc/stat` and memory utilization from `/proc/meminfo`.
- Process listing and name search from numeric `/proc/<pid>` directories, including resident memory (RSS).
- Safe default process termination with `SIGTERM`; `SIGKILL` requires `--force`.
- Process creation using `fork()`, `execvp()`, and `waitpid()`.
- Periodic CPU monitoring in a pthread with mutex-protected shared state and joined shutdown.
- Error handling for inaccessible or disappearing processes, invalid PIDs, permissions, and failed commands.

## Architecture

The code is split into small modules: `cpu_monitor` and `memory_monitor` read system statistics; `process_monitor` owns `/proc` scanning; `process_manager` sends signals; `process_launcher` owns fork/exec/wait; and `monitor_thread` manages the periodic worker. `main.c` provides the CLI.

## Dependencies

Linux, a C compiler, GNU Make, and POSIX pthread support. No third-party libraries are required.

## Build and run

```sh
make build
make run
make test
make clean
```

Commands:

```text
./syswatch                 # CPU and memory snapshot
./syswatch list [name]     # list processes, optionally filtered by name
./syswatch search <name>   # search process names
./syswatch terminate <pid> [--force]
./syswatch run <command> [args...]
./syswatch watch [seconds]
```

Process names and RSS values are read while scanning. A process may exit between directory enumeration and file reads; SysWatch skips that process and continues scanning.

## WSL limitations

WSL provides a Linux-compatible `/proc`, so the core monitor normally works. The process list represents the WSL environment rather than the complete Windows host, and Windows processes cannot be managed through Linux signals. Resource values can differ from Windows Task Manager. Some restricted containers or WSL configurations may also deny access to particular `/proc/<pid>` files.

## Testing

`make test` builds the program and runs smoke tests for help output, process listing/search, `fork`/`execvp`/`waitpid`, and invalid-argument handling. Process management tests should be run against a deliberately created, non-critical test process because signals affect real processes.
