/* skeleton.c */
#include <linux/init.h>        /* needed for macros */
#include <linux/io.h>          /* needed for mmio handling */
#include <linux/ioport.h>      /* needed for memory region handling */
#include <linux/kernel.h>      /* needed for debugging */
#include <linux/list.h>        /* needed for linked list processing */
#include <linux/module.h>      /* needed by all modules */
#include <linux/moduleparam.h> /* needed for module parameters */
#include <linux/slab.h>        /* needed for dynamic memory allocation */
#include <linux/string.h>      /* needed for string handling */

#define CHIPID_BASE 0x01c14000
#define CHIPID_OFFSET_1 0x200
#define CHIPID_OFFSET_2 0x204
#define CHIPID_OFFSET_3 0x208
#define CHIPID_OFFSET_4 0x20c

#define TEMP_BASE 0x01C25000
#define TEMP_OFFSET 0x80

#define MAC_BASE 0x01C30000
#define MAC_OFFSET_1 0x50
#define MAC_OFFSET_2 0x54

#define PAGE_SIZE 0x1000

static struct resource* res[3] = {
    [0] = 0,
};

unsigned char* regs[3] = {
    [0] = 0,
};

static int __init skeleton_init(void)
{
    pr_info("Linux module 05 skeleton loaded\n");

    // Informe CPU that we are using the memory region
    res[0] = request_mem_region(CHIPID_BASE, PAGE_SIZE, "Chip ID area");
    res[1] =
        request_mem_region(TEMP_BASE, PAGE_SIZE, "Temperature sensor area");
    res[2] = request_mem_region(MAC_BASE, PAGE_SIZE, "Ethernet MAC area");

    if ((res[0] == 0) || (res[1] == 0) || (res[2] == 0)) {
        pr_warn(
            "Error while reserving memory region... [0]=%d, [1]=%d, [2]=%d\n",
            res[0] == 0,
            res[1] == 0,
            res[2] == 0);
    }

    // Map the memory region to a virtual address
    regs[0] = ioremap(CHIPID_BASE, PAGE_SIZE);
    regs[1] = ioremap(TEMP_BASE, PAGE_SIZE);
    regs[2] = ioremap(MAC_BASE, PAGE_SIZE);

    if ((regs[0] == 0) || (regs[1] == 0) || (regs[2] == 0)) {
        pr_err("Error while trying to map processor register...\n");
        return -EFAULT;
    }

    // Read the chip ID
    unsigned int chipid[4] = {
        [0] = 0,
    };
    chipid[0] = ioread32(regs[0] + CHIPID_OFFSET_1);
    chipid[1] = ioread32(regs[0] + CHIPID_OFFSET_2);
    chipid[2] = ioread32(regs[0] + CHIPID_OFFSET_3);
    chipid[3] = ioread32(regs[0] + CHIPID_OFFSET_4);

    pr_info("chipid=%08x'%08x'%08x'%08x\n",
            chipid[0],
            chipid[1],
            chipid[2],
            chipid[3]);

    // Read the temperature
    long temperature = 0;
    temperature = -1191 * (int)ioread32(regs[1] + TEMP_OFFSET) / 10 + 223000;

    pr_info(
        "temperature=%ld (%d)\n", temperature, ioread32(regs[1] + TEMP_OFFSET));

    // Read the MAC address
    unsigned int addr[2] = {
        [0] = 0,
    };
    addr[0] = ioread32(regs[2] + MAC_OFFSET_1);
    addr[1] = ioread32(regs[2] + MAC_OFFSET_2);

    pr_info("mac-addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
            (addr[1] >> 0) & 0xff,
            (addr[1] >> 8) & 0xff,
            (addr[1] >> 16) & 0xff,
            (addr[1] >> 24) & 0xff,
            (addr[0] >> 0) & 0xff,
            (addr[0] >> 8) & 0xff);

    return 0;
}

static void __exit skeleton_exit(void)
{
    pr_info("Linux module skeleton unloaded\n");
    if (regs[0] != 0) iounmap(regs[0]);
    if (regs[1] != 0) iounmap(regs[1]);
    if (regs[2] != 0) iounmap(regs[2]);

    if (res[0] != 0) release_mem_region(CHIPID_BASE, PAGE_SIZE);
    if (res[1] != 0) release_mem_region(TEMP_BASE, PAGE_SIZE);
    if (res[2] != 0) release_mem_region(MAC_BASE, PAGE_SIZE);
}

module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
