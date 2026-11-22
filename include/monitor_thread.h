#ifndef MONITOR_THREAD_H
#define MONITOR_THREAD_H

#include <pthread.h>

struct MonitorThread {
    pthread_t thread;
    pthread_mutex_t mutex;
    int stop_requested;
    int running;
    double cpu_usage;
};

int monitor_thread_start(struct MonitorThread *monitor);
int monitor_thread_stop(struct MonitorThread *monitor);
double monitor_thread_cpu(struct MonitorThread *monitor);

#endif
