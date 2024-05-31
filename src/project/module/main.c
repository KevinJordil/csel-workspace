#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/moduleparam.h>  // needed for module parameters
#include <linux/sysfs.h>
#include <linux/thermal.h>
#include <linux/timer.h>

#define LED_GPIO 10

static struct timer_list blink_timer;
static unsigned int blink_frequency           = 2;
static unsigned int automatic_blink_frequency = 2;
static unsigned int manual_blink_frequency    = 2;
static bool automatic_mode                    = true;
static struct kobject* kobj_ref;

static ssize_t mode_automatic_show(struct kobject* kobj,
                                   struct kobj_attribute* attr,
                                   char* buf);
static ssize_t mode_automatic_store(struct kobject* kobj,
                                    struct kobj_attribute* attr,
                                    const char* buf,
                                    size_t count);
static ssize_t frequency_show(struct kobject* kobj,
                              struct kobj_attribute* attr,
                              char* buf);
static ssize_t frequency_store(struct kobject* kobj,
                               struct kobj_attribute* attr,
                               const char* buf,
                               size_t count);

static void update_blink_frequency(void);

// sysfs
static struct kobj_attribute mode_automatic_attr =
    __ATTR(mode_automatic, 0660, mode_automatic_show, mode_automatic_store);
static struct kobj_attribute frequency_attr =
    __ATTR(frequency, 0660, frequency_show, frequency_store);

// sysfs functions
static ssize_t mode_automatic_show(struct kobject* kobj,
                                   struct kobj_attribute* attr,
                                   char* buf)
{
    return sprintf(buf, "%s\n", automatic_mode ? "1" : "0");
}

static ssize_t mode_automatic_store(struct kobject* kobj,
                                    struct kobj_attribute* attr,
                                    const char* buf,
                                    size_t count)
{
    // 1 for automatic mode, 0 for manual mode
    if (buf[0] == '1') {
        automatic_mode = true;
    } else if (buf[0] == '0') {
        automatic_mode = false;
    } else {
        return -EINVAL;
    }
    return count;
}

static ssize_t frequency_show(struct kobject* kobj,
                              struct kobj_attribute* attr,
                              char* buf)
{
    return sprintf(buf, "%u\n", blink_frequency);
}

static ssize_t frequency_store(struct kobject* kobj,
                               struct kobj_attribute* attr,
                               const char* buf,
                               size_t count)
{
    int freq;
    if (kstrtoint(buf, 10, &freq) == 0 && freq > 0) {
        manual_blink_frequency = freq;
    } else {
        return -EINVAL;
    }
    return count;
}

static void blink_timer_callback(struct timer_list* timer)
{
    static bool led_value = false;
    led_value             = !led_value;
    gpio_set_value(LED_GPIO, led_value);
    // if automatic mode
    if (automatic_mode) {
        // Update blink frequency based on CPU temperature
        update_blink_frequency();
        blink_frequency = automatic_blink_frequency;
    } else {
        blink_frequency = manual_blink_frequency;
    }
    mod_timer(&blink_timer, jiffies + HZ / blink_frequency / 2);
}

static int get_cpu_temperature(void)
{
    struct thermal_zone_device* tz;
    int temp;

    tz = thermal_zone_get_zone_by_name("cpu-thermal");
    if (IS_ERR(tz)) {
        pr_err("Unable to get CPU thermal zone\n");
        return -1;
    }

    if (thermal_zone_get_temp(tz, &temp)) {
        pr_err("Unable to get CPU temperature\n");
        return -1;
    }

    return temp / 1000;
}

static void update_blink_frequency(void)
{
    int temp = get_cpu_temperature();
    if (temp < 0) {
        return;
    }

    if (temp < 35) {
        automatic_blink_frequency = 2;
    } else if (temp < 40) {
        automatic_blink_frequency = 5;
    } else if (temp < 45) {
        automatic_blink_frequency = 10;
    } else {
        automatic_blink_frequency = 20;
    }
}

static int __init led_blink_init(void)
{
    int ret;

    ret = gpio_request(LED_GPIO, "LED");
    if (ret) {
        pr_err("Failed to request GPIO %d for LED\n", LED_GPIO);
        return ret;
    }

    gpio_direction_output(LED_GPIO, 0);

    timer_setup(&blink_timer, blink_timer_callback, 0);
    mod_timer(&blink_timer, jiffies + HZ / blink_frequency / 2);

    // sysfs
    kobj_ref = kobject_create_and_add("led_blink_control", kernel_kobj);
    if (!kobj_ref) {
        pr_err("Failed to create kobject\n");
        gpio_free(LED_GPIO);
        return -ENOMEM;
    }

    ret = sysfs_create_file(kobj_ref, &mode_automatic_attr.attr);
    if (ret) {
        pr_err("Failed to create mode_automatic sysfs file\n");
        kobject_put(kobj_ref);
        gpio_free(LED_GPIO);
        return ret;
    }

    ret = sysfs_create_file(kobj_ref, &frequency_attr.attr);
    if (ret) {
        pr_err("Failed to create frequency sysfs file\n");
        sysfs_remove_file(kobj_ref, &mode_automatic_attr.attr);
        kobject_put(kobj_ref);
        gpio_free(LED_GPIO);
        return ret;
    }

    pr_info("LED blink module loaded\n");
    return 0;
}

static void __exit led_blink_exit(void)
{
    del_timer(&blink_timer);
    gpio_set_value(LED_GPIO, 0);
    gpio_free(LED_GPIO);
    sysfs_remove_file(kobj_ref, &mode_automatic_attr.attr);
    sysfs_remove_file(kobj_ref, &frequency_attr.attr);
    kobject_put(kobj_ref);
    pr_info("LED blink module unloaded\n");
}

module_init(led_blink_init);
module_exit(led_blink_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kévin Jordil & Mickaël Junod");
MODULE_DESCRIPTION("CSEL - Mini Projet");
MODULE_VERSION("1.0");
