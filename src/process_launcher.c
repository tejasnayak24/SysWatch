#include "process_launcher.h"

#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int launch_command(char *const argv[], int *exit_status) {
    if (argv == NULL || argv[0] == NULL || exit_status == NULL) {
        errno = EINVAL;
        return -1;
    }
    pid_t child = fork();
    if (child < 0) {
        return -1;
    }
    if (child == 0) {
        execvp(argv[0], argv);
        perror("execvp");
        _exit(127);
    }
    while (waitpid(child, exit_status, 0) < 0) {
        if (errno != EINTR) {
            return -1;
        }
    }
    return 0;
}
