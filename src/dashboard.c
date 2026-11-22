#include "dashboard.h"

#include "cpu_monitor.h"

#include <errno.h>
#include <stdlib.h>
#include <time.h>

static void free_snapshot_contents(struct DashboardSnapshot *snapshot) {
    free(snapshot->processes);
    snapshot->processes = NULL;
    snapshot->process_count = 0;
}

void dashboard_snapshot_free(struct DashboardSnapshot *snapshot) {
    if (snapshot != NULL) {
        free_snapshot_contents(snapshot);
    }
}

static int stop_requested(struct DashboardState *state) {
    pthread_mutex_lock(&state->mutex);
    int stop = state->stop_requested;
    pthread_mutex_unlock(&state->mutex);
    return stop;
}

static void *dashboard_worker(void *argument) {
    struct DashboardState *state = argument;
    while (!stop_requested(state)) {
        struct DashboardSnapshot next = {0};
        next.cpu_usage = get_cpu_usage();
        (void)get_memory_info(&next.memory);
        struct ProcessList processes;
        if (process_list_all(&processes) == 0) {
            next.processes = processes.items;
            next.process_count = processes.count;
        }
        pthread_mutex_lock(&state->mutex);
        if (state->stop_requested) {
            pthread_mutex_unlock(&state->mutex);
            dashboard_snapshot_free(&next);
            break;
        }
        free_snapshot_contents(&state->snapshot);
        state->snapshot.cpu_usage = next.cpu_usage;
        state->snapshot.memory = next.memory;
        state->snapshot.processes = next.processes;
        state->snapshot.process_count = next.process_count;
        state->snapshot.generation++;
        pthread_mutex_unlock(&state->mutex);
        struct timespec pause = {.tv_sec = 0, .tv_nsec = 250000000L};
        nanosleep(&pause, NULL);
    }
    return NULL;
}

int dashboard_start(struct DashboardState *state) {
    if (state == NULL) {
        return EINVAL;
    }
    state->snapshot.cpu_usage = -1.0;
    state->snapshot.processes = NULL;
    state->snapshot.process_count = 0;
    state->snapshot.generation = 0;
    state->stop_requested = 0;
    state->running = 0;
    int result = pthread_mutex_init(&state->mutex, NULL);
    if (result != 0) {
        return result;
    }
    result = pthread_create(&state->thread, NULL, dashboard_worker, state);
    if (result != 0) {
        pthread_mutex_destroy(&state->mutex);
        return result;
    }
    state->running = 1;
    return 0;
}

int dashboard_copy_snapshot(struct DashboardState *state,
                            struct DashboardSnapshot *snapshot) {
    if (state == NULL || snapshot == NULL) {
        return EINVAL;
    }
    snapshot->processes = NULL;
    snapshot->process_count = 0;
    pthread_mutex_lock(&state->mutex);
    snapshot->cpu_usage = state->snapshot.cpu_usage;
    snapshot->memory = state->snapshot.memory;
    snapshot->generation = state->snapshot.generation;
    if (state->snapshot.process_count > 0) {
        snapshot->processes = malloc(state->snapshot.process_count * sizeof(*snapshot->processes));
        if (snapshot->processes == NULL) {
            pthread_mutex_unlock(&state->mutex);
            return ENOMEM;
        }
        snapshot->process_count = state->snapshot.process_count;
        for (size_t index = 0; index < snapshot->process_count; index++) {
            snapshot->processes[index] = state->snapshot.processes[index];
        }
    }
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int dashboard_stop(struct DashboardState *state) {
    if (state == NULL || !state->running) {
        return EINVAL;
    }
    pthread_mutex_lock(&state->mutex);
    state->stop_requested = 1;
    pthread_mutex_unlock(&state->mutex);
    int result = pthread_join(state->thread, NULL);
    pthread_mutex_lock(&state->mutex);
    free_snapshot_contents(&state->snapshot);
    pthread_mutex_unlock(&state->mutex);
    pthread_mutex_destroy(&state->mutex);
    state->running = 0;
    return result;
}
