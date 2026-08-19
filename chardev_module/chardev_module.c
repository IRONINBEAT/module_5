#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/minmax.h>

#define DEVICE_NAME       "chardev_io"
#define CLASS_NAME        "chardev_io_class"
#define CHARDEV_BUF_SIZE  256

enum {
	DEV_NOT_USED = 0,
	DEV_EXCLUSIVE_OPEN = 1,
};

static int major_number;
static struct class *chardev_class;
static struct device *chardev_device;

static char *msg_buffer;
static size_t msg_len;

static atomic_t already_open = ATOMIC_INIT(DEV_NOT_USED);

static int device_open(struct inode *inode, struct file *file)
{
	if (atomic_cmpxchg(&already_open, DEV_NOT_USED, DEV_EXCLUSIVE_OPEN))
		return -EBUSY;

	try_module_get(THIS_MODULE);
	return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
	atomic_set(&already_open, DEV_NOT_USED);
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t device_read(struct file *filp, char __user *buffer,
			    size_t length, loff_t *offset)
{
	size_t remaining;
	size_t to_copy;

	if (*offset < 0 || (size_t)*offset >= msg_len)
		return 0;

	remaining = msg_len - *offset;
	to_copy = min(length, remaining);

	if (copy_to_user(buffer, msg_buffer + *offset, to_copy))
		return -EFAULT;

	*offset += to_copy;
	return to_copy;
}

static ssize_t device_write(struct file *filp, const char __user *buffer,
			     size_t length, loff_t *offset)
{
	loff_t pos = *offset;

	if (pos < 0 || pos >= CHARDEV_BUF_SIZE - 1)
		return -ENOSPC;

	if (length > CHARDEV_BUF_SIZE - 1 - pos)
		length = CHARDEV_BUF_SIZE - 1 - pos;

	if (copy_from_user(msg_buffer + pos, buffer, length))
		return -EFAULT;

	pos += length;
	msg_buffer[pos] = '\0';
	msg_len = pos;
	*offset = pos;

	return length;
}

static const struct file_operations chardev_fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.read = device_read,
	.write = device_write,
};

static int __init chardev_module_init(void)
{
	msg_buffer = kzalloc(CHARDEV_BUF_SIZE, GFP_KERNEL);
	if (!msg_buffer)
		return -ENOMEM;

	major_number = register_chrdev(0, DEVICE_NAME, &chardev_fops);
	if (major_number < 0) {
		pr_alert("chardev_io: registering char device failed with %d\n",
			 major_number);
		kfree(msg_buffer);
		return major_number;
	}

#ifdef CLASS_CREATE_SINGLE_ARG
	chardev_class = class_create(CLASS_NAME);
#else
	chardev_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
	if (IS_ERR(chardev_class)) {
		unregister_chrdev(major_number, DEVICE_NAME);
		kfree(msg_buffer);
		return PTR_ERR(chardev_class);
	}

	chardev_device = device_create(chardev_class, NULL,
					MKDEV(major_number, 0), NULL,
					DEVICE_NAME);
	if (IS_ERR(chardev_device)) {
		class_destroy(chardev_class);
		unregister_chrdev(major_number, DEVICE_NAME);
		kfree(msg_buffer);
		return PTR_ERR(chardev_device);
	}

	pr_info("chardev_io: loaded, major number %d, /dev/%s\n",
		major_number, DEVICE_NAME);
	return 0;
}

static void __exit chardev_module_exit(void)
{
	device_destroy(chardev_class, MKDEV(major_number, 0));
	class_destroy(chardev_class);
	unregister_chrdev(major_number, DEVICE_NAME);
	kfree(msg_buffer);
	pr_info("chardev_io: unloaded\n");
}

module_init(chardev_module_init);
module_exit(chardev_module_exit);

MODULE_LICENSE("VoroninLicense");
MODULE_AUTHOR("Ilya Voronin <ilyavoron2004@gmail.com>");
MODULE_DESCRIPTION("Символьное устройство для обмена данными с userspace через /dev");
MODULE_VERSION("1.0");
