/* skeleton.c */
#include <linux/delay.h>       /* needed for delay fonctions */
#include <linux/init.h>        /* needed for macros */
#include <linux/io.h>          /* needed for mmio handling */
#include <linux/ioport.h>      /* needed for memory region handling */
#include <linux/kernel.h>      /* needed for debugging */
#include <linux/kthread.h>     /* needed for kernel thread management */
#include <linux/list.h>        /* needed for linked list processing */
#include <linux/module.h>      /* needed by all modules */
#include <linux/moduleparam.h> /* needed for module parameters */
#include <linux/slab.h>        /* needed for dynamic memory allocation */
#include <linux/string.h>      /* needed for string handling */

struct task_struct* module_thread;

// thread function
static int module_thread_function(void* data)
{
    pr_info("Module 06 thread is now active...\n");
    while (!kthread_should_stop()) {
        ssleep(5);
        pr_info("module thread is kick every 5 seconds...\n");
    }
    return 0;
}

static int __init skeleton_init(void)
{
    pr_info("Linux module 06 loading...\n");

    module_thread = kthread_run(module_thread_function, 0, "module_thread");

    pr_info("Linux module 06 loaded\n");

    return 0;
}

static void __exit skeleton_exit(void)
{
    pr_info("Linux module skeleton unloading...\n");

    kthread_stop(module_thread);

    pr_info("Linux module skeleton unloaded\n");
}

module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
