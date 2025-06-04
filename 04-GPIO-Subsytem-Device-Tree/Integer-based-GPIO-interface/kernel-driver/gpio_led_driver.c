/**
 * @file gpio_led_driver.c
 * @brief GPIO LED character device driver using legacy GPIO interface
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include "gpio_led_driver.h"

/**
 * @brief GPIO LED driver device structure
 */
struct gpio_led_device {
    dev_t dev_num;
    struct class *device_class;
    struct device *device;
    struct cdev char_device;
    int gpio_pin;
    bool current_state;
    struct mutex state_mutex;
    bool gpio_requested;
};

/* Global device instance */
static struct gpio_led_device *gpio_led_dev;

/**
 * @brief Request and configure GPIO pin
 */
static int gpio_led_setup_pin(struct gpio_led_device *dev, int pin)
{
    int ret;
    
    /* Free previous GPIO if requested */
    if (dev->gpio_requested) {
        gpio_free(dev->gpio_pin);
        dev->gpio_requested = false;
    }
    
    /* Request new GPIO pin */
    ret = gpio_request(pin, "gpio_led");
    if (ret) {
        pr_err("GPIO_LED: Failed to request GPIO%d: %d\n", pin, ret);
        return ret;
    }
    
    /* Set as output */
    ret = gpio_direction_output(pin, GPIO_LOW);
    if (ret) {
        pr_err("GPIO_LED: Failed to set GPIO%d as output: %d\n", pin, ret);
        gpio_free(pin);
        return ret;
    }
    
    dev->gpio_pin = pin;
    dev->gpio_requested = true;
    dev->current_state = false;
    
    pr_info("GPIO_LED: GPIO%d configured as output\n", pin);
    return 0;
}

/**
 * @brief Set GPIO state
 */
static void gpio_led_set_state(struct gpio_led_device *dev, bool state)
{
    if (!dev->gpio_requested) {
        pr_warn("GPIO_LED: GPIO not configured\n");
        return;
    }
    
    gpio_set_value(dev->gpio_pin, state ? GPIO_HIGH : GPIO_LOW);
    dev->current_state = state;
    pr_info("GPIO_LED: GPIO%d set to %s\n", dev->gpio_pin, state ? "HIGH" : "LOW");
}

/**
 * @brief Get GPIO state
 */
static bool gpio_led_get_state(struct gpio_led_device *dev)
{
    if (!dev->gpio_requested) {
        pr_warn("GPIO_LED: GPIO not configured\n");
        return false;
    }
    
    dev->current_state = gpio_get_value(dev->gpio_pin);
    return dev->current_state;
}

/**
 * @brief Device open operation
 */
static int gpio_led_open(struct inode *inode, struct file *file)
{
    pr_info("GPIO_LED: Device opened\n");
    return 0;
}

/**
 * @brief Device release operation
 */
static int gpio_led_release(struct inode *inode, struct file *file)
{
    pr_info("GPIO_LED: Device closed\n");
    return 0;
}

/**
 * @brief Device read operation - return LED status
 */
static ssize_t gpio_led_read(struct file *file, char __user *buffer, 
                            size_t len, loff_t *offset)
{
    char status_msg[64];
    int msg_len;
    bool state;
    
    if (*offset > 0) 
        return 0;
    
    mutex_lock(&gpio_led_dev->state_mutex);
    state = gpio_led_get_state(gpio_led_dev);
    
    msg_len = snprintf(status_msg, sizeof(status_msg), 
                      "GPIO_LED: %s (GPIO%d)\n", 
                      state ? "ON" : "OFF", 
                      gpio_led_dev->gpio_pin);
    mutex_unlock(&gpio_led_dev->state_mutex);
    
    if (len < msg_len)
        return -EINVAL;
    
    if (copy_to_user(buffer, status_msg, msg_len))
        return -EFAULT;
    
    *offset += msg_len;
    return msg_len;
}

/**
 * @brief Device write operation - control LED
 */
static ssize_t gpio_led_write(struct file *file, const char __user *buffer,
                             size_t len, loff_t *offset)
{
    char command;
    
    if (len < 1)
        return -EINVAL;
    
    if (copy_from_user(&command, buffer, 1))
        return -EFAULT;
    
    mutex_lock(&gpio_led_dev->state_mutex);
    
    switch (command) {
    case LED_CMD_ON:
        gpio_led_set_state(gpio_led_dev, true);
        break;
        
    case LED_CMD_OFF:
        gpio_led_set_state(gpio_led_dev, false);
        break;
        
    case LED_CMD_TOGGLE:
        gpio_led_set_state(gpio_led_dev, !gpio_led_dev->current_state);
        break;
        
    default:
        pr_warn("GPIO_LED: Invalid command '%c'\n", command);
        mutex_unlock(&gpio_led_dev->state_mutex);
        return -EINVAL;
    }
    
    mutex_unlock(&gpio_led_dev->state_mutex);
    return len;
}

/**
 * @brief Device ioctl operation
 */
static long gpio_led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    int value;
    
    switch (cmd) {
    case GPIO_LED_IOC_SET_PIN:
        if (copy_from_user(&value, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        
        mutex_lock(&gpio_led_dev->state_mutex);
        ret = gpio_led_setup_pin(gpio_led_dev, value);
        mutex_unlock(&gpio_led_dev->state_mutex);
        break;
        
    case GPIO_LED_IOC_GET_PIN:
        mutex_lock(&gpio_led_dev->state_mutex);
        value = gpio_led_dev->gpio_pin;
        mutex_unlock(&gpio_led_dev->state_mutex);
        
        if (copy_to_user((int __user *)arg, &value, sizeof(int)))
            return -EFAULT;
        break;
        
    case GPIO_LED_IOC_SET_STATE:
        if (copy_from_user(&value, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        
        mutex_lock(&gpio_led_dev->state_mutex);
        gpio_led_set_state(gpio_led_dev, value ? true : false);
        mutex_unlock(&gpio_led_dev->state_mutex);
        break;
        
    case GPIO_LED_IOC_GET_STATE:
        mutex_lock(&gpio_led_dev->state_mutex);
        value = gpio_led_get_state(gpio_led_dev) ? 1 : 0;
        mutex_unlock(&gpio_led_dev->state_mutex);
        
        if (copy_to_user((int __user *)arg, &value, sizeof(int)))
            return -EFAULT;
        break;
        
    default:
        return -ENOTTY;
    }
    
    return ret;
}

/* File operations structure */
static struct file_operations gpio_led_fops = {
    .owner          = THIS_MODULE,
    .open           = gpio_led_open,
    .read           = gpio_led_read,
    .write          = gpio_led_write,
    .unlocked_ioctl = gpio_led_ioctl,
    .release        = gpio_led_release,
};

/**
 * @brief Module initialization
 */
static int __init gpio_led_init(void)
{
    int result;
    
    pr_info("GPIO_LED: Initializing driver (legacy GPIO interface)\n");
    
    /* Allocate device structure */
    gpio_led_dev = kzalloc(sizeof(*gpio_led_dev), GFP_KERNEL);
    if (!gpio_led_dev) {
        pr_err("GPIO_LED: Failed to allocate device structure\n");
        return -ENOMEM;
    }
    
    /* Initialize device data */
    gpio_led_dev->gpio_pin = DEFAULT_LED_GPIO;
    gpio_led_dev->current_state = false;
    gpio_led_dev->gpio_requested = false;
    mutex_init(&gpio_led_dev->state_mutex);
    
    /* Setup GPIO pin */
    result = gpio_led_setup_pin(gpio_led_dev, DEFAULT_LED_GPIO);
    if (result) {
        kfree(gpio_led_dev);
        return result;
    }
    
    /* Allocate device number */
    result = alloc_chrdev_region(&gpio_led_dev->dev_num, 0, 1, DEVICE_NAME);
    if (result < 0) {
        pr_err("GPIO_LED: Failed to allocate device number\n");
        goto cleanup_gpio;
    }
    
    /* Initialize character device */
    cdev_init(&gpio_led_dev->char_device, &gpio_led_fops);
    gpio_led_dev->char_device.owner = THIS_MODULE;
    
    result = cdev_add(&gpio_led_dev->char_device, gpio_led_dev->dev_num, 1);
    if (result < 0) {
        pr_err("GPIO_LED: Failed to add character device\n");
        goto cleanup_chrdev_region;
    }
    
    /* Create device class */
    gpio_led_dev->device_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(gpio_led_dev->device_class)) {
        pr_err("GPIO_LED: Failed to create device class\n");
        result = PTR_ERR(gpio_led_dev->device_class);
        goto cleanup_cdev;
    }
    
    /* Create device */
    gpio_led_dev->device = device_create(gpio_led_dev->device_class, NULL, 
                                        gpio_led_dev->dev_num, 
                                        NULL, DEVICE_NAME);
    if (IS_ERR(gpio_led_dev->device)) {
        pr_err("GPIO_LED: Failed to create device\n");
        result = PTR_ERR(gpio_led_dev->device);
        goto cleanup_class;
    }
    
    pr_info("GPIO_LED: Driver loaded - /dev/%s created (major:%d, minor:%d, GPIO%d)\n", 
            DEVICE_NAME, MAJOR(gpio_led_dev->dev_num), MINOR(gpio_led_dev->dev_num), 
            gpio_led_dev->gpio_pin);
    
    return 0;
    
cleanup_class:
    class_destroy(gpio_led_dev->device_class);
cleanup_cdev:
    cdev_del(&gpio_led_dev->char_device);
cleanup_chrdev_region:
    unregister_chrdev_region(gpio_led_dev->dev_num, 1);
cleanup_gpio:
    if (gpio_led_dev->gpio_requested)
        gpio_free(gpio_led_dev->gpio_pin);
    kfree(gpio_led_dev);
    pr_err("GPIO_LED: Driver initialization failed\n");
    return result;
}

/**
 * @brief Module cleanup
 */
static void __exit gpio_led_exit(void)
{
    if (!gpio_led_dev)
        return;
    
    pr_info("GPIO_LED: Unloading driver\n");
    
    /* Turn off LED */
    mutex_lock(&gpio_led_dev->state_mutex);
    if (gpio_led_dev->gpio_requested) {
        gpio_led_set_state(gpio_led_dev, false);
        gpio_free(gpio_led_dev->gpio_pin);
    }
    mutex_unlock(&gpio_led_dev->state_mutex);
    
    /* Cleanup */
    device_destroy(gpio_led_dev->device_class, gpio_led_dev->dev_num);
    class_destroy(gpio_led_dev->device_class);
    cdev_del(&gpio_led_dev->char_device);
    unregister_chrdev_region(gpio_led_dev->dev_num, 1);
    kfree(gpio_led_dev);
    
    pr_info("GPIO_LED: Driver unloaded successfully\n");
}

module_init(gpio_led_init);
module_exit(gpio_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TungNHS");
MODULE_DESCRIPTION("GPIO LED Character Device Driver using Legacy GPIO Interface for Raspberry Pi Zero W");
MODULE_VERSION("1.0");