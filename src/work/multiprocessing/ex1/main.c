#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void signal_handler(int signo)
{
    printf("Ignore signal %d\n", signo);
    // Do nothing
}

int main(void)
{
    // Create a socket pair
    int fd[2];
    int err = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    if (err == -1) {
        printf("Error: socketpair\n");
        return 1;
    }
    static const int parentsocket = 0;
    static const int childsocket  = 1;

    // Capture and ignore SIGHUP, SIGINT, SIGQUIT, SIGABRT and SIGTERM
    struct sigaction act = {
        .sa_handler = signal_handler,
    };
    err = sigaction(SIGHUP, &act, NULL);
    if (err == -1) {
        printf("Error: sigaction SIGHUP\n");
        return 1;
    }
    err = sigaction(SIGINT, &act, NULL);
    if (err == -1) {
        printf("Error: sigaction SIGINT\n");
        return 1;
    }
    err = sigaction(SIGQUIT, &act, NULL);
    if (err == -1) {
        printf("Error: sigaction SIGQUIT\n");
        return 1;
    }
    err = sigaction(SIGABRT, &act, NULL);
    if (err == -1) {
        printf("Error: sigaction SIGABRT\n");
        return 1;
    }
    err = sigaction(SIGTERM, &act, NULL);
    if (err == -1) {
        printf("Error: sigaction SIGTERM\n");
        return 1;
    }

    // Create a new child process
    pid_t pid = fork();

    if (pid > 0) {  // Parent process

        close(fd[childsocket]);

        // Set the affinity of the parent process to CPU 0
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(0, &set);
        int ret = sched_setaffinity(0, sizeof(set), &set);
        if (ret == -1) {
            printf("Error: sched_setaffinity\n");
            return 1;
        }

        // Wait message from the child process
        char buf[1024];
        int n = read(fd[0], buf, sizeof(buf));
        printf("Parent process received: '%.*s'\n", n, buf);

        n = read(fd[0], buf, sizeof(buf));
        printf("Parent process received: '%.*s'\n", n, buf);

        // Wait for the child process to finish
        int status;
        waitpid(pid, &status, 0);

    } else if (pid == 0) {  // Child process

        close(fd[parentsocket]);

        // Set the affinity of the child process to CPU 1
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(1, &set);
        int ret = sched_setaffinity(0, sizeof(set), &set);
        if (ret == -1) {
            printf("Error: sched_setaffinity\n");
            return 1;
        }

        // Send a message using a socket pair
        char msg[] = "Hello, World!";
        ret        = write(fd[1], msg, sizeof(msg));
        if (ret == -1) {
            printf("Error: write \"Hello, World!\"\n");
            return 1;
        }

        sleep(0.1);

        // Send a second message using a socket pair
        char msg2[] = "Goodbye, World!";
        ret         = write(fd[1], msg2, sizeof(msg2));
        if (ret == -1) {
            printf("Error: write \"Goodbye, World!\"\n");
            return 1;
        }

    } else {
        return 1;
    }
}