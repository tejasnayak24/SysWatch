
#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

struct MemoryInfo {
    unsigned long total_kb;
    unsigned long available_kb;
    unsigned long used_kb;
};

int get_memory_info(struct MemoryInfo *info);
void get_memory_usage(void);

#endif
