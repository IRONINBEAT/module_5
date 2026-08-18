#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/errno.h>

#define PROC_NAME "hello_proc"
#define PROC_PERMS 0644
#define PROC_BUFFER_SIZE 256

static struct proc_dir_entry *proc_entry;
static char *proc_buffer;
static size_t proc_buffer_len;

static ssize_t proc_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos)
{
	if (*ppos > 0 || proc_buffer_len == 0)
		return 0;

	if (count > proc_buffer_len)
		count = proc_buffer_len;

	if (copy_to_user(buf, proc_buffer, count))
		return -EFAULT;

	*ppos = count;
	return count;
}

static ssize_t proc_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	if (count > PROC_BUFFER_SIZE - 1)
		count = PROC_BUFFER_SIZE - 1;

	if (copy_from_user(proc_buffer, buf, count))
		return -EFAULT;

	proc_buffer[count] = '\0';
	proc_buffer_len = count;

	return count;
}

static const struct proc_ops proc_fops = {
	.proc_read = proc_read,
	.proc_write = proc_write,
};

static int __init proc_module_init(void)
{
	proc_buffer = kzalloc(PROC_BUFFER_SIZE, GFP_KERNEL);
	if (!proc_buffer)
		return -ENOMEM;

	proc_entry = proc_create(PROC_NAME, PROC_PERMS, NULL, &proc_fops);
	if (!proc_entry) {
		kfree(proc_buffer);
		return -ENOMEM;
	}

	pr_info("proc_module: /proc/%s created\n", PROC_NAME);
	return 0;
}

static void __exit proc_module_exit(void)
{
	proc_remove(proc_entry);
	kfree(proc_buffer);
	pr_info("proc_module: /proc/%s removed\n", PROC_NAME);
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("VoroninIlyaLicense");
MODULE_AUTHOR("Ilya Voronin <ilyavoron2004@gmail.com>");
MODULE_DESCRIPTION("Модуль ядра для обмена данными с userspace через /proc");
MODULE_VERSION("1.0");
