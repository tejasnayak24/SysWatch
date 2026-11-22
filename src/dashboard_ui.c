#include "dashboard_ui.h"

#include "dashboard.h"
#include "process_manager.h"

#include <curses.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

enum {
    COLOR_TITLE = 1,
    COLOR_SECTION,
    COLOR_BAR,
    COLOR_SELECTED,
    COLOR_MUTED,
    COLOR_ERROR
};

static volatile sig_atomic_t dashboard_interrupted;

static void handle_dashboard_interrupt(int signal_number) {
    (void)signal_number;
    dashboard_interrupted = 1;
}

static void draw_bar(int row, int column, int width, double ratio) {
    if (width <= 0) {
        return;
    }
    if (ratio < 0.0) {
        ratio = 0.0;
    } else if (ratio > 1.0) {
        ratio = 1.0;
    }
    int filled = (int)(ratio * width);
    attron(COLOR_PAIR(COLOR_BAR));
    for (int index = 0; index < width; index++) {
        mvaddch(row, column + index, index < filled ? ' ' : ACS_CKBOARD);
    }
    attroff(COLOR_PAIR(COLOR_BAR));
}

static void draw_header(void) {
    attron(COLOR_PAIR(COLOR_TITLE) | A_BOLD);
    mvprintw(0, 2, "SYSWATCH  //  LINUX SYSTEM MONITOR");
    attroff(COLOR_PAIR(COLOR_TITLE) | A_BOLD);
    attron(COLOR_PAIR(COLOR_MUTED));
    mvprintw(0, COLS - 28 > 2 ? COLS - 28 : 2, "q quit  ↑↓ select  k stop");
    attroff(COLOR_PAIR(COLOR_MUTED));
}

static void draw_dashboard(const struct DashboardSnapshot *snapshot,
                           size_t selected, const char *message) {
    erase();
    draw_header();
    if (LINES < 14 || COLS < 50) {
        mvprintw(2, 2, "Terminal too small. Resize to at least 50 columns x 14 rows.");
        refresh();
        return;
    }
    int bar_width = COLS > 55 ? COLS - 43 : 12;
    attron(COLOR_PAIR(COLOR_SECTION) | A_BOLD);
    mvprintw(2, 2, "SYSTEM HEALTH");
    mvprintw(5, 2, "MEMORY");
    mvprintw(8, 2, "PROCESSES  (%zu)", snapshot->process_count);
    attroff(COLOR_PAIR(COLOR_SECTION) | A_BOLD);

    double cpu = snapshot->cpu_usage;
    mvprintw(3, 2, "CPU Usage");
    if (cpu >= 0.0) {
        mvprintw(3, 15, "%6.2f%%", cpu);
        draw_bar(3, 25, bar_width, cpu / 100.0);
    } else {
        mvprintw(3, 15, " loading");
    }
    unsigned long total = snapshot->memory.total_kb;
    unsigned long used = snapshot->memory.used_kb;
    unsigned long available = snapshot->memory.available_kb;
    double memory_ratio = total > 0 ? (double)used / total : 0.0;
    mvprintw(6, 2, "Used");
    if (total > 0) {
        mvprintw(6, 15, "%6.2f%%", memory_ratio * 100.0);
        draw_bar(6, 25, bar_width, memory_ratio);
        mvprintw(7, 2, "%.1f GB used  /  %.1f GB total  /  %.1f GB available",
                 used / 1048576.0, total / 1048576.0, available / 1048576.0);
    } else {
        mvprintw(6, 15, " loading");
    }

    int table_top = 9;
    int table_bottom = LINES - 3;
    attron(A_BOLD);
    mvprintw(table_top, 2, "%-8s %-32s %12s", "PID", "PROCESS", "RSS (KB)");
    attroff(A_BOLD);
    int visible_rows = table_bottom - table_top - 1;
    size_t first = 0;
    if (visible_rows > 0 && selected >= (size_t)visible_rows) {
        first = selected - (size_t)visible_rows + 1;
    }
    for (int row = 0; row < visible_rows; row++) {
        size_t index = first + (size_t)row;
        if (index >= snapshot->process_count) {
            break;
        }
        const struct ProcessInfo *process = &snapshot->processes[index];
        if (index == selected) {
            attron(COLOR_PAIR(COLOR_SELECTED));
        }
        mvprintw(table_top + 1 + row, 2, "%-8ld %-32.32s %12lu",
                 (long)process->pid, process->name, process->resident_kb);
        if (index == selected) {
            attroff(COLOR_PAIR(COLOR_SELECTED));
        }
    }
    attron(COLOR_PAIR(COLOR_MUTED));
    mvprintw(LINES - 2, 2, "Enter k to send SIGTERM; K sends SIGKILL only after confirmation.");
    if (message != NULL) {
        attron(COLOR_PAIR(COLOR_ERROR) | A_BOLD);
        mvprintw(LINES - 1, 2, "%.*s", COLS - 4 > 0 ? COLS - 4 : 1, message);
        attroff(COLOR_PAIR(COLOR_ERROR) | A_BOLD);
    }
    attroff(COLOR_PAIR(COLOR_MUTED));
    refresh();
}

static int confirm_action(const struct ProcessInfo *process, int force) {
    char prompt[128];
    snprintf(prompt, sizeof(prompt), "%s PID %ld (%.80s)? [y/N] ",
             force ? "KILL" : "Terminate", (long)process->pid, process->name);
    attron(COLOR_PAIR(COLOR_ERROR) | A_BOLD);
    mvprintw(LINES - 1, 2, "%.*s", COLS - 4 > 0 ? COLS - 4 : 1, prompt);
    attroff(COLOR_PAIR(COLOR_ERROR) | A_BOLD);
    refresh();
    nodelay(stdscr, FALSE);
    int key = getch();
    nodelay(stdscr, TRUE);
    return key == 'y' || key == 'Y';
}

int run_dashboard(void) {
    struct sigaction action = {0};
    struct sigaction previous_action;
    action.sa_handler = handle_dashboard_interrupt;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, &previous_action) != 0) {
        return 1;
    }
    struct DashboardState state;
    int result = dashboard_start(&state);
    if (result != 0) {
        sigaction(SIGINT, &previous_action, NULL);
        fprintf(stderr, "Unable to start dashboard collector: %s\n", strerror(result));
        return 1;
    }
    if (initscr() == NULL) {
        (void)dashboard_stop(&state);
        sigaction(SIGINT, &previous_action, NULL);
        return 1;
    }
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(COLOR_TITLE, COLOR_CYAN, -1);
        init_pair(COLOR_SECTION, COLOR_GREEN, -1);
        init_pair(COLOR_BAR, COLOR_BLUE, -1);
        init_pair(COLOR_SELECTED, COLOR_BLACK, COLOR_CYAN);
        init_pair(COLOR_MUTED, COLOR_WHITE, -1);
        init_pair(COLOR_ERROR, COLOR_RED, -1);
    }
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    size_t selected = 0;
    char message[160] = "Starting collector...";
    while (!dashboard_interrupted) {
        struct DashboardSnapshot snapshot;
        result = dashboard_copy_snapshot(&state, &snapshot);
        if (result != 0) {
            snprintf(message, sizeof(message), "Snapshot error: %s", strerror(result));
            napms(200);
            continue;
        }
        if (snapshot.process_count == 0) {
            selected = 0;
        } else if (selected >= snapshot.process_count) {
            selected = snapshot.process_count - 1;
        }
        draw_dashboard(&snapshot, selected, message);
        message[0] = '\0';
        int key = getch();
        if (key == 'q' || key == 'Q' || key == 27) {
            dashboard_snapshot_free(&snapshot);
            break;
        } else if (key == KEY_UP && selected > 0) {
            selected--;
        } else if (key == KEY_DOWN && selected + 1 < snapshot.process_count) {
            selected++;
        } else if (key == 'k' || key == 'K') {
            if (snapshot.process_count == 0) {
                snprintf(message, sizeof(message), "No process is selected.");
            } else if (confirm_action(&snapshot.processes[selected], key == 'K')) {
                if (terminate_process(snapshot.processes[selected].pid, key == 'K') != 0) {
                    snprintf(message, sizeof(message), "Unable to signal PID %ld: %s",
                             (long)snapshot.processes[selected].pid, strerror(errno));
                } else {
                    snprintf(message, sizeof(message), "Signal sent to PID %ld.",
                             (long)snapshot.processes[selected].pid);
                }
            } else {
                snprintf(message, sizeof(message), "Action cancelled.");
            }
        }
        dashboard_snapshot_free(&snapshot);
        napms(150);
    }
    endwin();
    result = dashboard_stop(&state);
    sigaction(SIGINT, &previous_action, NULL);
    if (result != 0) {
        fprintf(stderr, "Unable to stop dashboard collector: %s\n", strerror(result));
        return 1;
    }
    return 0;
}
