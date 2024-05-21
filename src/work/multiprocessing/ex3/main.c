#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    pid_t high_pid = fork();

    if (high_pid == 0) {  // Child process
        // Print the PID of the child process
        printf("High priority process PID: %d\n", getpid());

        while (1) {
        }
        return 0;
    } else if (high_pid < 0) {
        printf("Error: high_pid fork\n");
        return 1;
    }

    pid_t low_pid = fork();

    if (low_pid == 0) {  // Child process
        // Print the PID of the child process
        printf("Low priority process PID: %d\n", getpid());

        while (1) {
        }
        return 0;
    } else if (low_pid < 0) {
        printf("Error: low_pid fork\n");
        return 1;
    }

    waitpid(high_pid, NULL, 0);
    waitpid(low_pid, NULL, 0);

    return 0;
}
