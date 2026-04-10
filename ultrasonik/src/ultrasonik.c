#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/ktime.h>
#include <linux/math64.h>

struct ultrasonik_dev {
    struct gpio_desc *trig;
    struct gpio_desc *echo;

    dev_t devt;
    struct cdev cdev;
    struct class *class;
    struct device *device;

    struct mutex lock;
    int delta_us;
};

static int ultrasonik_measure(struct ultrasonik_dev *usdev)
{
    u64 t;
    ktime_t t1, t2;

    /* trigger pulse */
    gpiod_set_value_cansleep(usdev->trig, 0);
    usleep_range(2, 5);

    gpiod_set_value_cansleep(usdev->trig, 1);
    usleep_range(15, 25);
    gpiod_set_value_cansleep(usdev->trig, 0);

    /* wait for echo rising edge */
    t = ktime_get_ns();
    while (gpiod_get_value_cansleep(usdev->echo) == 0) {
        if (ktime_get_ns() - t > 30000000) { /* 30 ms timeout */
            dev_err(usdev->device, "timeout waiting for echo rising edge\n");
            return -ETIMEDOUT;
        }
        cpu_relax();
    }

    t1 = ktime_get();

    /* wait for echo falling edge */
	t = ktime_get_ns();
	while (gpiod_get_value_cansleep(usdev->echo) == 1) {
    	if (ktime_get_ns() - t > 100000000000) {
        	dev_err(usdev->device, "timeout waiting for echo falling edge (high for %llu us)\n", div_u64(ktime_get_ns() - t, 1000));
        return -ETIMEDOUT;
    }
    cpu_relax();
}

    t2 = ktime_get();
    usdev->delta_us = (int)ktime_us_delta(t2, t1);

    dev_info(usdev->device, "echo pulse width = %d us\n", usdev->delta_us);

    return 0;
}

static int dev_open(struct inode *ino, struct file *fp)
{
    struct ultrasonik_dev *usdev;

    usdev = container_of(ino->i_cdev, struct ultrasonik_dev, cdev);
    fp->private_data = usdev;

    return 0;
}

static int dev_release(struct inode *ino, struct file *fp)
{
    return 0;
}

static ssize_t dev_read(struct file *fp, char __user *buf, size_t n, loff_t *ppos)
{
    struct ultrasonik_dev *usdev = fp->private_data;
    char msg[32];
    int len;
    int ret;
    int distance_cm;

    if (*ppos != 0)
        return 0;

    mutex_lock(&usdev->lock);

    ret = ultrasonik_measure(usdev);
    if (ret) {
        mutex_unlock(&usdev->lock);
        return ret;
    }

    distance_cm = usdev->delta_us / 58;
    len = scnprintf(msg, sizeof(msg), "%d\n", distance_cm);

    mutex_unlock(&usdev->lock);

    if (len > n)
        len = n;

    if (copy_to_user(buf, msg, len))
        return -EFAULT;

    *ppos += len;
    return len;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .release = dev_release,
};

static int gpio_init_probe(struct platform_device *pdev)
{
    int ret;
    struct ultrasonik_dev *usdev;

    dev_info(&pdev->dev, "ultrasonik probe\n");

    usdev = devm_kzalloc(&pdev->dev, sizeof(*usdev), GFP_KERNEL);
    if (!usdev)
        return -ENOMEM;

    mutex_init(&usdev->lock);

    usdev->trig = devm_gpiod_get(&pdev->dev, "pin-trig", GPIOD_OUT_LOW);
    if (IS_ERR(usdev->trig))
        return PTR_ERR(usdev->trig);

    usdev->echo = devm_gpiod_get(&pdev->dev, "pin-echo", GPIOD_IN);
    if (IS_ERR(usdev->echo))
        return PTR_ERR(usdev->echo);

    ret = alloc_chrdev_region(&usdev->devt, 0, 1, "ultrasonik");
    if (ret)
        return ret;

    cdev_init(&usdev->cdev, &fops);
    usdev->cdev.owner = THIS_MODULE;

    ret = cdev_add(&usdev->cdev, usdev->devt, 1);
    if (ret)
        goto err_unregister;

    usdev->class = class_create("ultrasonik_class");
    if (IS_ERR(usdev->class)) {
        ret = PTR_ERR(usdev->class);
        goto err_cdev_del;
    }

    usdev->device = device_create(usdev->class, NULL, usdev->devt, NULL, "ultrasonik");
    if (IS_ERR(usdev->device)) {
        ret = PTR_ERR(usdev->device);
        goto err_class_destroy;
    }

    platform_set_drvdata(pdev, usdev);

    dev_info(&pdev->dev, "created /dev/ultrasonik\n");
    return 0;

err_class_destroy:
    class_destroy(usdev->class);
err_cdev_del:
    cdev_del(&usdev->cdev);
err_unregister:
    unregister_chrdev_region(usdev->devt, 1);
    return ret;
}

static int gpio_exit_remove(struct platform_device *pdev)
{
    struct ultrasonik_dev *usdev = platform_get_drvdata(pdev);

    device_destroy(usdev->class, usdev->devt);
    class_destroy(usdev->class);
    cdev_del(&usdev->cdev);
    unregister_chrdev_region(usdev->devt, 1);

    dev_info(&pdev->dev, "ultrasonik remove\n");
    return 0;
}

static const struct of_device_id ultrasonik_match[] = {
    { .compatible = "insa,ultrasonik" },
    { }
};
MODULE_DEVICE_TABLE(of, ultrasonik_match);

static struct platform_driver ultrasonik = {
    .probe = gpio_init_probe,
    .remove = gpio_exit_remove,
    .driver = {
        .name = "ultrasonik",
        .of_match_table = ultrasonik_match,
    }
};

module_platform_driver(ultrasonik);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ultrasonik device");
MODULE_AUTHOR("Le J & Ivanopolis");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:ultrasonik");
