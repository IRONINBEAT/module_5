#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/tty.h>            /* fg_console, MAX_NR_CONSOLES */
#include <linux/kd.h>             /* KDSETLED */
#include <linux/vt.h>
#include <linux/console_struct.h> /* vc_cons */
#include <linux/vt_kern.h>
#include <linux/timer.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/errno.h>

#define BLINK_DELAY     (HZ / 5)

#define LED_SCROLLLOCK  0x1
#define LED_NUMLOCK     0x2
#define LED_CAPSLOCK    0x4
#define LED_MASK_ALL    (LED_SCROLLLOCK | LED_NUMLOCK | LED_CAPSLOCK)
#define LED_RESTORE     0xFF

#define SYSFS_DIR_NAME  "kbleds_blink"

static struct timer_list blink_timer;
static struct tty_driver *kbd_tty_driver;
static struct kobject *kbleds_kobject;

static int led_mask = LED_MASK_ALL;
static int led_blink_state;

static void blink_timer_func(struct timer_list *t)
{
	if (led_blink_state == led_mask)
		led_blink_state = LED_RESTORE;
	else
		led_blink_state = led_mask;

	kbd_tty_driver->ops->ioctl(vc_cons[fg_console].d->port.tty,
				    KDSETLED, led_blink_state);

	mod_timer(&blink_timer, jiffies + BLINK_DELAY);
}

static ssize_t mask_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	return sprintf(buf, "%d\n", led_mask);
}

static ssize_t mask_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	int value;
	int ret = kstrtoint(buf, 10, &value);

	if (ret)
		return ret;

	if (value < 0 || value > LED_MASK_ALL)
		return -EINVAL;

	led_mask = value;
	return count;
}

static struct kobj_attribute mask_attribute =
	__ATTR(mask, 0664, mask_show, mask_store);

static int __init kbleds_sysfs_init(void)
{
	int error;

	if (!vc_cons[fg_console].d || !vc_cons[fg_console].d->port.tty) {
		pr_err("kbleds_sysfs: no active console tty found\n");
		return -ENODEV;
	}
	kbd_tty_driver = vc_cons[fg_console].d->port.tty->driver;

	kbleds_kobject = kobject_create_and_add(SYSFS_DIR_NAME, kernel_kobj);
	if (!kbleds_kobject)
		return -ENOMEM;

	error = sysfs_create_file(kbleds_kobject, &mask_attribute.attr);
	if (error) {
		pr_err("kbleds_sysfs: failed to create sysfs file\n");
		kobject_put(kbleds_kobject);
		return error;
	}

	timer_setup(&blink_timer, blink_timer_func, 0);
	blink_timer.expires = jiffies + BLINK_DELAY;
	add_timer(&blink_timer);

	pr_info("kbleds_sysfs: loaded, control via /sys/kernel/%s/mask\n",
		SYSFS_DIR_NAME);
	return 0;
}

static void __exit kbleds_sysfs_exit(void)
{
	del_timer_sync(&blink_timer);

	kbd_tty_driver->ops->ioctl(vc_cons[fg_console].d->port.tty,
				    KDSETLED, LED_RESTORE);

	kobject_put(kbleds_kobject);
	pr_info("kbleds_sysfs: unloaded\n");
}

module_init(kbleds_sysfs_init);
module_exit(kbleds_sysfs_exit);

MODULE_LICENSE("VoroninLicense");
MODULE_AUTHOR("Ilya Voronin <ilyavoron2004@gmail.com>");
MODULE_DESCRIPTION("Мигание клавиатурных LED через ioctl KDSETLED, маска мигания задаётся через sysfs");
MODULE_VERSION("1.0");
