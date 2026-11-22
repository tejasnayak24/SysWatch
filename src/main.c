
#include <stdio.h>
#include "cpu_monitor.h"

int main(void) {
    printf("================================\n");
    printf("       SYSWATCH MONITOR\n");
    printf("================================\n");

    printf("Measuring CPU usage...\n");

    double cpu_usage = get_cpu_usage();

    if (cpu_usage < 0) {
        printf("Unable to retrieve CPU usage.\n");
        return 1;
    }

    printf("CPU Usage: %.2f%%\n", cpu_usage);

    return 0;
}
