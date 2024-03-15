/* skeleton.c */
#include <linux/module.h>	/* needed by all modules */
#include <linux/init.h>		/* needed for macros */
#include <linux/kernel.h>	/* needed for debugging */

#include <linux/moduleparam.h>	/* needed for module parameters */

#include <linux/slab.h>		/* needed for dynamic memory allocation */
#include <linux/list.h>		/* needed for linked list processing */
#include <linux/string.h>	/* needed for string handling */

#include <linux/ioport.h>	/* needed for memory region handling */
#include <linux/io.h>		/* needed for mmio handling */

#include <linux/kthread.h>	/* needed for kernel thread management */
#include <linux/delay.h>	/* needed for delay fonctions */


static struct task_struct* my_thread;


static int skeleton_thread (void* data)
{
	pr_info ("skeleton thread is now active...\n");
	while(!kthread_should_stop()) {
		ssleep (5);
		pr_info ("skeleton thread is kick every 5 seconds...\n");
	}
	return 0;
}


static int __init skeleton_init(void)
{
	pr_info ("Linux module 06 skeleton loaded\n");

	my_thread = kthread_run (skeleton_thread, 0, "s/thread");

	return 0;
}

static void __exit skeleton_exit(void)
{
	kthread_stop (my_thread);

	pr_info ("Linux module skeleton unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION ("Module skeleton");
MODULE_LICENSE ("GPL");

