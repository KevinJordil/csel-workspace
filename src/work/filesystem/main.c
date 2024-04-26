#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#define GPIO_EXPORT "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_LED "/sys/class/gpio/gpio10"
#define GPIO_BUTTON0 "/sys/class/gpio/gpio0"
#define GPIO_BUTTON0_EVENT "/sys/class/gpio/gpio0/edge"
#define GPIO_BUTTON1 "/sys/class/gpio/gpio2"
#define GPIO_BUTTON1_EVENT "/sys/class/gpio/gpio2/edge"
#define GPIO_BUTTON2 "/sys/class/gpio/gpio3"
#define GPIO_BUTTON2_EVENT "/sys/class/gpio/gpio3/edge"
#define LED "10"
#define BUTTON0 "0"
#define BUTTON1 "2"
#define BUTTON2 "3"

static int led_fd;
static int button_fds[3];

static void set_led_state(int state)
{
    if (state)
        write(led_fd, "1", sizeof("1"));
    else
        write(led_fd, "0", sizeof("0"));
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
    write(f, "rising", sizeof("rising"));
    close(f);

    f = open(GPIO_BUTTON1_EVENT, O_WRONLY);
    write(f, "rising", sizeof("rising"));
    close(f);

    f = open(GPIO_BUTTON2_EVENT, O_WRONLY);
    write(f, "rising", sizeof("rising"));
    close(f);

    // Open gpio value attributes for buttons
    button_fds[0] = open(GPIO_BUTTON0 "/value", O_RDONLY);
    button_fds[1] = open(GPIO_BUTTON1 "/value", O_RDONLY);
    button_fds[2] = open(GPIO_BUTTON2 "/value", O_RDONLY);
}

int main(int argc, char* argv[])
{
    long period = 500;  // ms
    if (argc >= 2) period = atoi(argv[1]);
    period *= 1000000;  // in ns

    // Initialize LED and buttons
    init_led();
    init_buttons();

    // Create timerfd
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timer_fd == -1) {
        perror("timerfd_create");
        return 1;
    }

    // Setup timer expiration
    struct itimerspec timer_spec;
    timer_spec.it_interval.tv_sec  = period / 1000000000;
    timer_spec.it_interval.tv_nsec = period % 1000000000;
    timer_spec.it_value            = timer_spec.it_interval;
    if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1) {
        perror("timerfd_settime");
        return 1;
    }

    // Create epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return 1;
    }

    // Add timer_fd and button fds to epoll
    struct epoll_event event;
    event.events  = EPOLLIN;
    event.data.fd = timer_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &event) == -1) {
        perror("epoll_ctl");
        return 1;
    }

    for (int i = 0; i < 3; ++i) {
        struct epoll_event event;
        event.events  = EPOLLIN;
        event.data.fd = button_fds[i];
        lseek(button_fds[i], 0, SEEK_SET);  // Consume any pending events
        char buffer[8];
        read(button_fds[i], buffer, sizeof(buffer));
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, button_fds[i], &event) == -1) {
            perror("epoll_ctl");
            return 1;
        }
    }

    // Event loop
    while (1) {
        struct epoll_event events[4];  // 3 buttons + timer
        int num_events = epoll_wait(epoll_fd, events, 4, -1);
        if (num_events == -1) {
            perror("epoll_wait");
            return 1;
        }

        // Handle events
        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == timer_fd) {
                uint64_t exp;
                static int state = 0;
                read(timer_fd, &exp, sizeof(exp));  // Consume the event
                state = !state;
                set_led_state(state);
            } else if (events[i].data.fd == button_fds[0]) {
                static int old_state_0 = 0;
                char buf;
                lseek(button_fds[0], 0, SEEK_SET);
                read(button_fds[0], &buf, 1);  // Consume the event
                if (buf == '1' && old_state_0 == 0) {
                    printf("Button 0 pressed\n");
                    old_state_0 = 1;
                } else if (buf == '0' && old_state_0 == 1) {
                    old_state_0 = 0;
                }
            } else if (events[i].data.fd == button_fds[1]) {
                static int old_state_1 = 0;
                char buf;
                lseek(button_fds[1], 0, SEEK_SET);
                read(button_fds[1], &buf, 1);  // Consume the event
                if (buf == '1' && old_state_1 == 0) {
                    printf("Button 1 pressed\n");
                    old_state_1 = 1;
                } else if (buf == '0' && old_state_1 == 1) {
                    old_state_1 = 0;
                }
            } else if (events[i].data.fd == button_fds[2]) {
                static int old_state_2 = 0;
                char buf;
                lseek(button_fds[2], 0, SEEK_SET);
                read(button_fds[2], &buf, 1);  // Consume the event
                if (buf == '1' && old_state_2 == 0) {
                    printf("Button 2 pressed\n");
                    old_state_2 = 1;
                } else if (buf == '0' && old_state_2 == 1) {
                    old_state_2 = 0;
                }
            }
        }
    }

    // Cleanup
    close(epoll_fd);
    close(timer_fd);
    close(led_fd);
    close(button_fds[0]);
    close(button_fds[1]);
    close(button_fds[2]);
    for (int i = 0; i < 3; ++i) close(button_fds[i]);

    return 0;
}
