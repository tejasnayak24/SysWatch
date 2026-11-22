#include "monitor_thread.h"

#include "cpu_monitor.h"

#include <errno.h>
#include <stdio.h>
#include <time.h>

static void *monitor_worker(void *argument) {
    struct MonitorThread *monitor = argument;
    for (;;) {
        double usage = get_cpu_usage();
        pthread_mutex_lock(&monitor->mutex);
        monitor->cpu_usage = usage;
        int stop = monitor->stop_requested;
        pthread_mutex_unlock(&monitor->mutex);
        if (stop) {
            break;
        }
        struct timespec pause = {.tv_sec = 1, .tv_nsec = 0};
        nanosleep(&pause, NULL);
    }
    return NULL;
}

int monitor_thread_start(struct MonitorThread *monitor) {
    if (monitor == NULL) {
        return EINVAL;
    }
    monitor->stop_requested = 0;
    monitor->running = 1;
    monitor->cpu_usage = -1.0;
    int result = pthread_mutex_init(&monitor->mutex, NULL);
    if (result != 0) {
        monitor->running = 0;
        return result;
    }
    result = pthread_create(&monitor->thread, NULL, monitor_worker, monitor);
    if (result != 0) {
        pthread_mutex_destroy(&monitor->mutex);
        monitor->running = 0;
    }
    return result;
}

int monitor_thread_stop(struct MonitorThread *monitor) {
    if (monitor == NULL || !monitor->running) {
        return EINVAL;
    }
    pthread_mutex_lock(&monitor->mutex);
    monitor->stop_requested = 1;
    pthread_mutex_unlock(&monitor->mutex);
    int result = pthread_join(monitor->thread, NULL);
    pthread_mutex_destroy(&monitor->mutex);
    monitor->running = 0;
    return result;
}

double monitor_thread_cpu(struct MonitorThread *monitor) {
    if (monitor == NULL) {
        return -1.0;
    }
    pthread_mutex_lock(&monitor->mutex);
    double usage = monitor->cpu_usage;
    pthread_mutex_unlock(&monitor->mutex);
    return usage;
}
