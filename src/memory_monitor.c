#include <stdio.h>
#include <string.h>

#include "memory_monitor.h"

int get_memory_info(struct MemoryInfo *info) {
    if (info == NULL) {
        return -1;
    }
    FILE *file = fopen("/proc/meminfo", "r");
    if (file == NULL) {
        return -1;
    }
    unsigned long total_memory = 0;
    unsigned long available_memory = 0;
    char label[100];
    unsigned long value;
    char unit[20];
    while (fscanf(file, "%99s %lu %19s", label, &value, unit) == 3) {
        if (strcmp(label, "MemTotal:") == 0) {
            total_memory = value;
        } else if (strcmp(label, "MemAvailable:") == 0) {
            available_memory = value;
        }
    }
    fclose(file);
    if (total_memory == 0 || available_memory > total_memory) {
        return -1;
    }
    info->total_kb = total_memory;
    info->available_kb = available_memory;
    info->used_kb = total_memory - available_memory;
    return 0;
}

void get_memory_usage(void) {
    struct MemoryInfo info;
    if (get_memory_info(&info) != 0) {
        printf("Unable to retrieve memory information.\n");
        return;
    }
    double usage_percentage = (double)info.used_kb / info.total_kb * 100.0;
    printf("\n----- MEMORY INFORMATION -----\n");
    printf("Total Memory:     %.2f GB\n", info.total_kb / 1048576.0);
    printf("Used Memory:      %.2f GB\n", info.used_kb / 1048576.0);
    printf("Available Memory: %.2f GB\n", info.available_kb / 1048576.0);
    printf("Memory Usage:     %.2f%%\n", usage_percentage);
}
