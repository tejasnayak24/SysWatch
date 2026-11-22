#include "process_monitor.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_pid_name(const char *name) {
    if (*name == '\0') {
        return 0;
    }
    for (const char *cursor = name; *cursor != '\0'; cursor++) {
        if (!isdigit((unsigned char)*cursor)) {
            return 0;
        }
    }
    return 1;
}

static int read_process_info(const char *pid_name, struct ProcessInfo *info) {
    char path[64];
    FILE *file;
    char line[4096];

    int written = snprintf(path, sizeof(path), "/proc/%s/comm", pid_name);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return -1;
    }
    file = fopen(path, "r");
    if (file == NULL || fgets(info->name, sizeof(info->name), file) == NULL) {
        if (file != NULL) {
            fclose(file);
        }
        return -1;
    }
    fclose(file);
    info->name[strcspn(info->name, "\n")] = '\0';

    written = snprintf(path, sizeof(path), "/proc/%s/status", pid_name);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return -1;
    }
    file = fopen(path, "r");
    if (file == NULL) {
        return -1;
    }
    info->resident_kb = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        unsigned long value;
        if (sscanf(line, "VmRSS: %lu kB", &value) == 1) {
            info->resident_kb = value;
            break;
        }
    }
    fclose(file);

    char *end = NULL;
    errno = 0;
    long parsed_pid = strtol(pid_name, &end, 10);
    if (errno != 0 || end == pid_name || *end != '\0' || parsed_pid <= 0) {
        return -1;
    }
    info->pid = (pid_t)parsed_pid;
    return 0;
}

int process_list_all(struct ProcessList *list) {
    if (list == NULL) {
        errno = EINVAL;
        return -1;
    }
    list->items = NULL;
    list->count = 0;

    DIR *proc = opendir("/proc");
    if (proc == NULL) {
        return -1;
    }
    struct dirent *entry;
    while ((entry = readdir(proc)) != NULL) {
        if (!is_pid_name(entry->d_name)) {
            continue;
        }
        struct ProcessInfo info;
        if (read_process_info(entry->d_name, &info) != 0) {
            /* A process can exit between readdir() and fopen(); skip it. */
            continue;
        }
        struct ProcessInfo *expanded = realloc(
            list->items, (list->count + 1) * sizeof(*list->items));
        if (expanded == NULL) {
            closedir(proc);
            process_list_free(list);
            return -1;
        }
        list->items = expanded;
        list->items[list->count++] = info;
    }
    closedir(proc);
    return 0;
}

void process_list_free(struct ProcessList *list) {
    if (list != NULL) {
        free(list->items);
        list->items = NULL;
        list->count = 0;
    }
}

void process_list_print(const struct ProcessList *list) {
    printf("%-8s %-30s %12s\n", "PID", "NAME", "RSS (KB)");
    printf("%-8s %-30s %12s\n", "--------", "------------------------------", "------------");
    for (size_t index = 0; index < list->count; index++) {
        printf("%-8ld %-30s %12lu\n", (long)list->items[index].pid,
               list->items[index].name, list->items[index].resident_kb);
    }
}

void process_list_print_matching(const struct ProcessList *list,
                                 const char *query) {
    for (size_t index = 0; index < list->count; index++) {
        if (strstr(list->items[index].name, query) != NULL) {
            printf("%-8ld %-30s %12lu\n", (long)list->items[index].pid,
                   list->items[index].name, list->items[index].resident_kb);
        }
    }
}
