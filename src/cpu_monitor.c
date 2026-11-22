
#include <stdio.h>
#include <unistd.h>

#include "cpu_monitor.h"

struct CpuTimes {
    unsigned long long total;
    unsigned long long idle;
};

// Read CPU statistics from /proc/stat
static int read_cpu_times(struct CpuTimes *times) {
    FILE *file = fopen("/proc/stat", "r");

    if (file == NULL) {
        perror("Unable to open /proc/stat");
        return -1;
    }

    unsigned long long user, nice, system, idle;
    unsigned long long iowait, irq, softirq, steal;
    unsigned long long guest, guest_nice;

    int result = fscanf(file,
        "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
        &user, &nice, &system, &idle, &iowait,
        &irq, &softirq, &steal, &guest, &guest_nice);

    fclose(file);

    if (result != 10) {
        fprintf(stderr, "Failed to read CPU statistics\n");
        return -1;
    }

    times->idle = idle + iowait;

    times->total = user + nice + system + idle +
               iowait + irq + softirq + steal;

    return 0;
}

// Calculate CPU utilization
double get_cpu_usage(void) {
    struct CpuTimes first, second;

    if (read_cpu_times(&first) != 0) {
        return -1.0;
    }

    sleep(1);

    if (read_cpu_times(&second) != 0) {
        return -1.0;
    }

    unsigned long long total_diff = second.total - first.total;
    unsigned long long idle_diff = second.idle - first.idle;

    if (total_diff == 0) {
        return -1.0;
    }

    double usage = 100.0 *
        (total_diff - idle_diff) / total_diff;

    return usage;
}
