#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#include "ssd1306.h"

#define GPIO_EXPORT "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_LED "/sys/class/gpio/gpio362"
#define GPIO_BUTTON0 "/sys/class/gpio/gpio0"
#define GPIO_BUTTON0_EVENT "/sys/class/gpio/gpio0/edge"
#define GPIO_BUTTON1 "/sys/class/gpio/gpio2"
#define GPIO_BUTTON1_EVENT "/sys/class/gpio/gpio2/edge"
#define GPIO_BUTTON2 "/sys/class/gpio/gpio3"
#define GPIO_BUTTON2_EVENT "/sys/class/gpio/gpio3/edge"
#define LED "362"
#define BUTTON0 "0"
#define BUTTON1 "2"
#define BUTTON2 "3"
#define MODE_PATH "/sys/kernel/led_blink_control/mode_automatic"
#define FREQ_PATH "/sys/kernel/led_blink_control/frequency"
#define MODE_AUTO "1"
#define MODE_MANUAL "0"
#define CPU_TEMP "/sys/class/thermal/thermal_zone0/temp"

#define DEFAULT_PERIOD 500  // ms

static int cpu_temp_fd;
static int mode_fd;
static int freq_fd;
static int led_fd;
static int button_fds[3];
static const char* mode = MODE_AUTO;

static void set_led_state(int state)
{
    if (state)
        write(led_fd, "1", sizeof("1"));
    else
        write(led_fd, "0", sizeof("0"));
}

static void init_cpu_temp()
{
    cpu_temp_fd = open(CPU_TEMP, O_RDONLY);
    if (cpu_temp_fd == -1) {
        syslog(LOG_ERR, "open failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void init_mode()
{
    mode_fd = open(MODE_PATH, O_RDWR);
    if (mode_fd == -1) {
        syslog(LOG_ERR, "open failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Get initial mode
    char buffer[16];
    lseek(mode_fd, 0, SEEK_SET);
    int len     = read(mode_fd, buffer, sizeof(buffer));
    buffer[len] = '\0';
    mode        = strdup(buffer);
    syslog(LOG_INFO, "Initial mode: %s", mode);
}

static void init_freq()
{
    freq_fd = open(FREQ_PATH, O_RDWR);
    if (freq_fd == -1) {
        syslog(LOG_ERR, "open failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Display initial frequency
    char buffer[16];
    lseek(freq_fd, 0, SEEK_SET);
    int len     = read(freq_fd, buffer, sizeof(buffer));
    buffer[len] = '\0';
    syslog(LOG_INFO, "Initial frequency: %s", buffer);
}

static void init_led()
{
    // Unexport pin out of sysfs (reinitialization)
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, LED, sizeof(LED));
    close(f);

    // Export pin to sysfs
    f = open(GPIO_EXPORT, O_WRONLY);
    write(f, LED, sizeof(LED));
    close(f);

    // Config pin
    f = open(GPIO_LED "/direction", O_WRONLY);
    write(f, "out", sizeof("out"));
    close(f);

    // Open gpio value attribute
    led_fd = open(GPIO_LED "/value", O_WRONLY);
}

static void init_buttons()
{
    // Export button GPIOs
    int f = open(GPIO_EXPORT, O_WRONLY);
    write(f, BUTTON0, sizeof(BUTTON0));
    write(f, BUTTON1, sizeof(BUTTON1));
    write(f, BUTTON2, sizeof(BUTTON2));
    close(f);

    // Config buttons
    f = open(GPIO_BUTTON0 "/direction", O_WRONLY);
    write(f, "in", sizeof("in"));
    close(f);

    f = open(GPIO_BUTTON1 "/direction", O_WRONLY);
    write(f, "in", sizeof("in"));
    close(f);

    f = open(GPIO_BUTTON2 "/direction", O_WRONLY);
    write(f, "in", sizeof("in"));
    close(f);

    // Event mode on click
    f = open(GPIO_BUTTON0_EVENT, O_WRONLY);
    write(f, "both", sizeof("both"));
    close(f);

    f = open(GPIO_BUTTON1_EVENT, O_WRONLY);
    write(f, "both", sizeof("both"));
    close(f);

    f = open(GPIO_BUTTON2_EVENT, O_WRONLY);
    write(f, "both", sizeof("both"));
    close(f);

    // Open gpio value attributes for buttons
    button_fds[0] = open(GPIO_BUTTON0 "/value", O_RDONLY);
    button_fds[1] = open(GPIO_BUTTON1 "/value", O_RDONLY);
    button_fds[2] = open(GPIO_BUTTON2 "/value", O_RDONLY);
}

static int get_cpu_temp()
{
    char buffer[16];
    lseek(cpu_temp_fd, 0, SEEK_SET);
    int len     = read(cpu_temp_fd, buffer, sizeof(buffer));
    buffer[len] = '\0';
    return atoi(buffer) / 1000;
}

// Dislay information on OLED
static void display_info(float period)
{
    ssd1306_clear_display();

    ssd1306_set_position(0, 0);
    ssd1306_puts("CSEL1a - SP.07");
    ssd1306_set_position(0, 1);
    ssd1306_puts("  Demo - SW");
    ssd1306_set_position(0, 2);
    ssd1306_puts("--------------");

    // Temperature
    ssd1306_set_position(0, 3);
    ssd1306_puts("Temp: ");
    char temp_buffer[16];
    int temp = get_cpu_temp();
    snprintf(temp_buffer, sizeof(temp_buffer), "%d C", temp);
    ssd1306_puts(temp_buffer);

    // Frequency
    ssd1306_set_position(0, 4);
    ssd1306_puts("Freq: ");
    char freq_buffer[16];
    int period_hz = 1000000000 / period;
    snprintf(freq_buffer, sizeof(freq_buffer), "%d", period_hz);
    ssd1306_puts(freq_buffer);
    ssd1306_puts("Hz");

    // Mode
    ssd1306_set_position(0, 5);
    ssd1306_puts("Mode: ");
    if (strcmp(mode, MODE_AUTO) == 0)
        ssd1306_puts("automatic");
    else
        ssd1306_puts("manual");
}

// Update led frequency
static int update_led_frequency(float period)
{
    char buffer[16];
    // Period from ns to Hz
    int period_hz = 1000000000 / period;
    snprintf(buffer, sizeof(buffer), "%d", period_hz);
    lseek(freq_fd, 0, SEEK_SET);
    return write(freq_fd, buffer, strlen(buffer));
}

// Switch led mode
static void switch_led_mode()
{
    const char* new_mode =
        (strcmp(mode, MODE_AUTO) == 0) ? MODE_MANUAL : MODE_AUTO;
    lseek(mode_fd, 0, SEEK_SET);
    write(mode_fd, new_mode, strlen(new_mode));
    mode = new_mode;
}

static void handle_signal(int signal)
{
    syslog(LOG_INFO, "Received signal %d, terminating...", signal);
    close(led_fd);
    close(cpu_temp_fd);
    close(mode_fd);
    close(freq_fd);
    close(cpu_temp_fd);
    close(button_fds[0]);
    close(button_fds[1]);
    close(button_fds[2]);
    ssd1306_clear_display();
    closelog();
    exit(0);
}

int main(int argc, char* argv[])
{
    // Fork and exit parent
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Create a new session
    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    // Fork again and exit parent
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Print PID
    printf("Deamon PID: %d\n", getpid());

    // Capture signals
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    // Update file creation mask
    umask(0);

    // Change working directory
    if (chdir("/") < 0) {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    // Close all file descriptors
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }

    // Redirect stdin, stdout, stderr to /dev/null
    int fd = open("/dev/null", O_RDWR);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    // Open syslog
    openlog("deamon_CSEL", LOG_PID, LOG_DAEMON);

    // Rest of the original main function
    float period = DEFAULT_PERIOD;  // ms
    if (argc >= 2) period = atoi(argv[1]);
    period *= 1000000;  // in ns

    // Initialize LED and buttons
    init_led();
    init_buttons();
    init_mode();
    init_freq();
    init_cpu_temp();
    ssd1306_init();

    display_info(period);

    // Setup timer expiration
    if (update_led_frequency(period) == -1) {
        syslog(LOG_ERR, "update_led_frequency failed");
        exit(EXIT_FAILURE);
    }

    // Create epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        syslog(LOG_ERR, "epoll_create1 failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 3; ++i) {
        struct epoll_event event;
        event.events  = EPOLLET;
        event.data.fd = button_fds[i];
        lseek(button_fds[i], 0, SEEK_SET);  // Consume any pending events
        char buffer[8];
        read(button_fds[i], buffer, sizeof(buffer));
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, button_fds[i], &event) == -1) {
            syslog(LOG_ERR, "epoll_ctl failed: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    char buf;
    int led_state  = 0;
    int btn1_state = 0;
    int btn2_state = 0;
    int btn3_state = 0;

    syslog(LOG_INFO, "Deamon ready !");

    // Event loop
    while (1) {
        struct epoll_event events[3];  // 3 buttons + timer
        int num_events = epoll_wait(epoll_fd, events, 3, -1);
        if (num_events == -1) {
            syslog(LOG_ERR, "epoll_wait failed: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Handle events
        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == button_fds[0]) {
                lseek(button_fds[0], 0, SEEK_SET);
                read(button_fds[0], &buf, 1);  // Consume the event
                if (btn1_state == 0) {
                    btn1_state = 1;

                    // Increase led blink frequency
                    period /= 2;
                    if (update_led_frequency(period) == -1) {
                        syslog(LOG_ERR, "update_led_frequency failed");
                        exit(EXIT_FAILURE);
                    }
                    display_info(period);
                    set_led_state(led_state = !led_state);
                    syslog(LOG_INFO, "Frequency increased");

                } else {
                    btn1_state = 0;
                    set_led_state(led_state = !led_state);
                }

            } else if (events[i].data.fd == button_fds[1]) {
                lseek(button_fds[2], 0, SEEK_SET);
                read(button_fds[2], &buf, 1);  // Consume the event
                if (btn2_state == 0) {
                    btn2_state = 1;
                    // if period is greater than 1Hz
                    if ((1000000000 / period) > 1) {
                        // Decrease led blink frequency
                        period *= 2;
                        if (update_led_frequency(period) == -1) {
                            syslog(LOG_ERR, "update_led_frequency failed");
                            exit(EXIT_FAILURE);
                        }
                        display_info(period);
                        set_led_state(led_state = !led_state);
                        syslog(LOG_INFO, "Frequency decreased");
                    }
                } else {
                    btn2_state = 0;
                    set_led_state(led_state = !led_state);
                }

            } else if (events[i].data.fd == button_fds[2]) {
                lseek(button_fds[2], 0, SEEK_SET);
                read(button_fds[2], &buf, 1);  // Consume the event
                if (btn3_state == 0) {
                    btn3_state = 1;
                    // Change led mode
                    switch_led_mode();
                    display_info(period);
                    set_led_state(led_state = !led_state);
                    syslog(LOG_INFO, "Mode switched\n");
                } else {
                    btn3_state = 0;
                    set_led_state(led_state = !led_state);
                }
            }
        }
    }

    // Cleanup
    close(led_fd);
    close(cpu_temp_fd);
    close(mode_fd);
    close(freq_fd);
    close(cpu_temp_fd);
    close(button_fds[0]);
    close(button_fds[1]);
    close(button_fds[2]);
    ssd1306_clear_display();
    closelog();

    return 0;
}
