#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <pthread.h>

#include "memory_monitor.h"
#include "process_monitor.h"

struct DashboardSnapshot {
    double cpu_usage;
    struct MemoryInfo memory;
    struct ProcessInfo *processes;
    size_t process_count;
    unsigned long generation;
};

struct DashboardState {
    pthread_t thread;
    pthread_mutex_t mutex;
    int stop_requested;
    int running;
    struct DashboardSnapshot snapshot;
};

int dashboard_start(struct DashboardState *state);
int dashboard_copy_snapshot(struct DashboardState *state,
                            struct DashboardSnapshot *snapshot);
void dashboard_snapshot_free(struct DashboardSnapshot *snapshot);
int dashboard_stop(struct DashboardState *state);

#endif
