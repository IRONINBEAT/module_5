#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("VoroninIlyaLicense");
MODULE_AUTHOR("Ilya Voronin <ilyavoron2004@gmail.com>");
MODULE_DESCRIPTION("Учебный модуль ядра Hello World для практики #1 модуля 5");
MODULE_VERSION("1.0");

static int __init hello_init(void)
{
	printk(KERN_INFO "Hello, World! Module loaded by Ilya Voronin.\n");
	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_INFO "Goodbye, World! Module unloaded.\n");
}

module_init(hello_init);
module_exit(hello_exit);
