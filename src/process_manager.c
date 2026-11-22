#include "process_manager.h"

#include <errno.h>
#include <signal.h>

int terminate_process(pid_t pid, int force) {
    if (pid <= 0) {
        errno = EINVAL;
        return -1;
    }
    return kill(pid, force ? SIGKILL : SIGTERM);
}
