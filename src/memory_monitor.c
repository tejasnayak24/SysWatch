#include <string.h>
#include <stdio.h>
#include "memory_monitor.h"

void get_memory_usage(void) {
    FILE *file = fopen("/proc/meminfo", "r");

    if (file == NULL) {
        perror("Unable to open /proc/meminfo");
        return;
    }

    unsigned long total_memory = 0;
    unsigned long available_memory = 0;

    char label[100];
    unsigned long value;
    char unit[20];

    while (fscanf(file, "%99s %lu %19s", label, &value, unit) == 3) {

        if (strcmp(label, "MemTotal:") == 0) {
            total_memory = value;
        }

        if (strcmp(label, "MemAvailable:") == 0) {
            available_memory = value;
        }
    }

    fclose(file);

    if (total_memory == 0) {
        printf("Unable to retrieve memory information.\n");
        return;
    }

    unsigned long used_memory = total_memory - available_memory;

    double usage_percentage =
        (double)used_memory / total_memory * 100.0;

    printf("\n----- MEMORY INFORMATION -----\n");
    printf("Total Memory:     %.2f GB\n", total_memory / 1048576.0);
    printf("Used Memory:      %.2f GB\n", used_memory / 1048576.0);
    printf("Available Memory: %.2f GB\n", available_memory / 1048576.0);
    printf("Memory Usage:     %.2f%%\n", usage_percentage);
}
