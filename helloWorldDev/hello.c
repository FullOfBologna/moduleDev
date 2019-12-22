#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define KBD_IRQ 0x1 //Keyboard irq for x86 architecture


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeff Atkinson");
MODULE_DESCRIPTION("Hello Kernel Application");
MODULE_VERSION("0.0");
/*
static irqreturn_t kbd_watcher(int irq, void dev_id*)
{

}
*/
static int __init mod_init(void)
{
	printk(KERN_CRIT "Hello, Linux!\n");
	return 0;
}

static void __exit mod_exit(void)
{
	printk(KERN_CRIT "Goodbye, Linux :(\n");
}

module_init(mod_init);
module_exit(mod_exit);
