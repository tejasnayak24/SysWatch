#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <sys/types.h>
#include <stddef.h>

#define PROCESS_NAME_LENGTH 256

struct ProcessInfo {
    pid_t pid;
    char name[PROCESS_NAME_LENGTH];
    unsigned long resident_kb;
};

struct ProcessList {
    struct ProcessInfo *items;
    size_t count;
};

int process_list_all(struct ProcessList *list);
void process_list_free(struct ProcessList *list);
void process_list_print(const struct ProcessList *list);
void process_list_print_matching(const struct ProcessList *list,
                                 const char *query);

#endif
