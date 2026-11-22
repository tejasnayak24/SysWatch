#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cpu_monitor.h"
#include "dashboard_ui.h"
#include "memory_monitor.h"
#include "monitor_thread.h"
#include "process_launcher.h"
#include "process_manager.h"
#include "process_monitor.h"

static volatile sig_atomic_t watch_interrupted;

static void handle_watch_interrupt(int signal_number) {
    (void)signal_number;
    watch_interrupted = 1;
}

static void print_usage(const char *program) {
    printf("Usage:\n");
    printf("  %s                         Open the interactive ncurses dashboard\n", program);
    printf("  %s snapshot                Show a non-interactive CPU/memory snapshot\n", program);
    printf("  %s list [name]             List or filter processes\n", program);
    printf("  %s search <name>           Search processes by name\n", program);
    printf("  %s terminate <pid> [--force]  Send SIGTERM (or explicit SIGKILL)\n", program);
    printf("  %s run <command> [args...]  Fork, exec and wait for a command\n", program);
    printf("  %s watch [seconds]          Monitor CPU periodically in a pthread\n", program);
}

static int list_processes(const char *query) {
    struct ProcessList list;
    if (process_list_all(&list) != 0) {
        perror("Unable to scan /proc");
        return 1;
    }
    if (query == NULL) {
        process_list_print(&list);
    } else {
        printf("%-8s %-30s %12s\n", "PID", "NAME", "RSS (KB)");
        printf("%-8s %-30s %12s\n", "--------", "------------------------------", "------------");
        process_list_print_matching(&list, query);
    }
    process_list_free(&list);
    return 0;
}

static int terminate_command(const char *pid_text, int force) {
    char *end = NULL;
    errno = 0;
    long parsed = strtol(pid_text, &end, 10);
    if (errno != 0 || end == pid_text || *end != '\0' || parsed <= 0) {
        fprintf(stderr, "Invalid PID: %s\n", pid_text);
        return 1;
    }
    if (terminate_process((pid_t)parsed, force) != 0) {
        fprintf(stderr, "Unable to terminate PID %ld: %s\n", parsed, strerror(errno));
        return 1;
    }
    printf("Sent %s to PID %ld.\n", force ? "SIGKILL" : "SIGTERM", parsed);
    return 0;
}

static int run_command(char *const command[]) {
    int status;
    if (launch_command(command, &status) != 0) {
        perror("Unable to launch command");
        return 1;
    }
    if (WIFEXITED(status)) {
        printf("Command exited with status %d.\n", WEXITSTATUS(status));
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        printf("Command terminated by signal %d.\n", WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }
    return 1;
}

static int watch_command(const char *duration_text) {
    long duration = 5;
    if (duration_text != NULL) {
        char *end = NULL;
        errno = 0;
        duration = strtol(duration_text, &end, 10);
        if (errno != 0 || *duration_text == '\0' || *end != '\0' || duration <= 0 || duration > 86400) {
            fprintf(stderr, "Watch duration must be between 1 and 86400 seconds.\n");
            return 1;
        }
    }
    struct sigaction interrupt_action = {0};
    struct sigaction previous_action;
    interrupt_action.sa_handler = handle_watch_interrupt;
    sigemptyset(&interrupt_action.sa_mask);
    if (sigaction(SIGINT, &interrupt_action, &previous_action) != 0) {
        perror("Unable to install SIGINT handler");
        return 1;
    }
    watch_interrupted = 0;
    struct MonitorThread monitor;
    int result = monitor_thread_start(&monitor);
    if (result != 0) {
        sigaction(SIGINT, &previous_action, NULL);
        fprintf(stderr, "Unable to start monitor thread: %s\n", strerror(result));
        return 1;
    }
    printf("Watching CPU usage for %ld seconds.\n", duration);
    for (long second = 0; second < duration && !watch_interrupted; second++) {
        sleep(1);
        double usage = monitor_thread_cpu(&monitor);
        if (usage >= 0.0) {
            printf("CPU Usage: %.2f%%\n", usage);
        } else {
            printf("CPU Usage: unavailable\n");
        }
    }
    result = monitor_thread_stop(&monitor);
    sigaction(SIGINT, &previous_action, NULL);
    if (result != 0) {
        fprintf(stderr, "Unable to stop monitor thread: %s\n", strerror(result));
        return 1;
    }
    return 0;
}

static int snapshot_command(void) {
    printf("================================\n");
    printf("       SYSWATCH MONITOR\n");
    printf("================================\n");
    printf("\nMeasuring CPU usage...\n");
    double cpu_usage = get_cpu_usage();
    if (cpu_usage < 0) {
        printf("Unable to retrieve CPU usage.\n");
        return 1;
    }
    printf("CPU Usage: %.2f%%\n", cpu_usage);
    get_memory_usage();
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "list") == 0) {
        if (argc > 3) { print_usage(argv[0]); return 1; }
        return list_processes(argc == 3 ? argv[2] : NULL);
    }
    if (argc > 1 && strcmp(argv[1], "search") == 0) {
        if (argc != 3) { print_usage(argv[0]); return 1; }
        return list_processes(argv[2]);
    }
    if (argc > 1 && strcmp(argv[1], "terminate") == 0) {
        if (argc < 3 || argc > 4 || (argc == 4 && strcmp(argv[3], "--force") != 0)) {
            print_usage(argv[0]); return 1;
        }
        return terminate_command(argv[2], argc == 4);
    }
    if (argc > 1 && strcmp(argv[1], "run") == 0) {
        if (argc < 3) { print_usage(argv[0]); return 1; }
        return run_command(&argv[2]);
    }
    if (argc > 1 && strcmp(argv[1], "watch") == 0) {
        if (argc > 3) { print_usage(argv[0]); return 1; }
        return watch_command(argc == 3 ? argv[2] : NULL);
    }
    if (argc > 1 && strcmp(argv[1], "snapshot") == 0) {
        if (argc != 2) { print_usage(argv[0]); return 1; }
        return snapshot_command();
    }
    if (argc > 1) { print_usage(argv[0]); return 1; }
    return run_dashboard();
}
