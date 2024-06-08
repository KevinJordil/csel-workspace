#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/led_blink_fifo"

void usage(const char* prog_name)
{
    fprintf(stderr, "Usage: %s <mode> [frequency]\n", prog_name);
    fprintf(stderr, "    <mode> can be 'automatic' or 'manual'\n");
    fprintf(stderr, "    [frequency] is required if mode is 'manual'\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 3) {
        usage(argv[0]);
    }

    char mode = 0;
    char message[256];

    if (strcmp(argv[1], "automatic") == 0) {
        mode = 'a';
        snprintf(message, sizeof(message), "%c", mode);
    } else if (strcmp(argv[1], "manual") == 0) {
        if (argc != 3) {
            usage(argv[0]);
        }
        mode = 'm';
        snprintf(message, sizeof(message), "%c%s", mode, argv[2]);
    } else {
        usage(argv[0]);
    }

    // Open the FIFO for writing
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Write the message to the FIFO
    if (write(fd, message, strlen(message)) == -1) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Print Switched to automatic/manual mode
    if (mode == 'a') {
        printf("Switched to automatic mode\n");
    } else {
        printf("Switched to manual mode with frequency of %sHz\n", argv[2]);
    }

    close(fd);
    return 0;
}
