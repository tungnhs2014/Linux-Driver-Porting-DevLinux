/**
 * @file gpio_led_driver.c
 * @brief GPIO Descriptor LED Driver Implementation - C90 Compliant
 * @author TungNHS
 * @version 1.0
 * 
 * Clean implementation with:
 * - C90 standard compliance
 * - GPIO descriptor API (modern approach)
 * - Device Tree integration
 * - Thread-safe operations
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include "gpio_led_driver.h"

/**
 * @brief Main driver context structure
 */
struct led_driver_ctx {
    /* Platform device */
    struct platform_device *pdev;
    
    /* Character device */
    struct device *dev_node;
    struct class *dev_class;
    struct cdev cdev;
    dev_t dev_num;
    
    /* GPIO descriptors - Modern approach */
    struct gpio_desc *led_gpios[MAX_LEDS];
    const char *led_names[MAX_LEDS];
    bool led_states[MAX_LEDS];
    int led_count;
    int current_led;
    
    /* Synchronization */
    struct mutex lock;
};

/* Global driver instance */
static struct led_driver_ctx *g_ctx;

/* Function prototypes */
static int gpio_led_probe(struct platform_device *pdev);
static int gpio_led_remove(struct platform_device *pdev);
static int gpio_led_open(struct inode *inode, struct file *file);
static int gpio_led_release(struct inode *inode, struct file *file);
static ssize_t gpio_led_read(struct file *file, char __user *buf, 
                            size_t count, loff_t *pos);
static ssize_t gpio_led_write(struct file *file, const char __user *buf, 
                             size_t count, loff_t *pos);
static long gpio_led_ioctl(struct file *file, unsigned int cmd, 
                          unsigned long arg);

/**
 * @brief Device Tree matching table
 */
static const struct of_device_id gpio_led_dt_match[] = {
    { .compatible = "custom,gpio-led-descriptor" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, gpio_led_dt_match);

/**
 * @brief Platform device ID table for non-Device Tree platforms
 */
static const struct platform_device_id gpio_led_id_table[] = {
    { "gpio-led-descriptor", 0 },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(platform, gpio_led_id_table);

/**
 * @brief Platform driver structure with modern registration
 */
static struct platform_driver gpio_led_driver = {
    .probe = gpio_led_probe,
    .remove = gpio_led_remove,
    .id_table = gpio_led_id_table,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = gpio_led_dt_match,
        .owner = THIS_MODULE,
    },
};

/**
 * @brief File operations structure
 */
static const struct file_operations gpio_led_fops = {
    .owner = THIS_MODULE,
    .open = gpio_led_open,
    .release = gpio_led_release,
    .read = gpio_led_read,
    .write = gpio_led_write,
    .unlocked_ioctl = gpio_led_ioctl,
};

/**
 * @brief Set LED state using GPIO descriptor
 */
static void led_set_state(int led_idx, bool state)
{
    if ((led_idx >= 0) && (led_idx < g_ctx->led_count) && (g_ctx->led_gpios[led_idx])) {
        gpiod_set_value(g_ctx->led_gpios[led_idx], state ? 1 : 0);
        g_ctx->led_states[led_idx] = state;

        dev_info(&g_ctx->pdev->dev, "LED %d (%s): %s\n", led_idx, g_ctx->led_names[led_idx], state ? "ON" : "OFF");
    }
}

/**
 * @brief Get LED current state
 */
static bool led_get_state(int led_idx)
{
    if ((led_idx >= 0) && (led_idx < g_ctx->led_count) && (g_ctx->led_gpios[led_idx])) {
        return gpiod_get_value(g_ctx->led_gpios[led_idx]) ? true : false;
    }
    return false;
}

/**
 * @brief Parse Device Tree configuration - C90 Compliant
 */
static int parse_dt_config(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *node = dev->of_node;
    int ret;
    int i;
    u32 led_count;
    const char *names[MAX_LEDS];  /* Moved to top */
    
    dev_info(dev, "Parsing Device Tree configuration\n");
    
    /* Get LED count */
    ret = of_property_read_u32(node, "num-leds", &led_count);
    if (ret) {
        dev_err(dev, "Failed to read num-leds: %d\n", ret);
        return ret;
    }
    
    g_ctx->led_count = min((int)led_count, MAX_LEDS);
    dev_info(dev, "Configuring %d LEDs\n", g_ctx->led_count);
    
    /* Get GPIO descriptors */
    if (g_ctx->led_count >= 1) {
        g_ctx->led_gpios[0] = devm_gpiod_get(dev, "status-led", GPIOD_OUT_LOW);
        if (IS_ERR(g_ctx->led_gpios[0])) {
            dev_err(dev, "Failed to get status LED GPIO\n");
            return PTR_ERR(g_ctx->led_gpios[0]);
        }
        dev_info(dev, "Status LED GPIO acquired\n");
    }
    
    if (g_ctx->led_count >= 2) {
        g_ctx->led_gpios[1] = devm_gpiod_get_optional(dev, "power-led", GPIOD_OUT_LOW);
        if (IS_ERR(g_ctx->led_gpios[1])) {
            dev_warn(dev, "Power LED GPIO not available\n");
            g_ctx->led_gpios[1] = NULL;
            g_ctx->led_count = 1;
        } else if (g_ctx->led_gpios[1]) {
            dev_info(dev, "Power LED GPIO acquired\n");
        }
    }
    
    /* Parse LED names */
    ret = of_property_read_string_array(node, "led-names", names, g_ctx->led_count);
    if (ret < 0) {
        /* Default names */
        g_ctx->led_names[0] = "status";
        g_ctx->led_names[1] = "power";
        dev_info(dev, "Using default LED names\n");
    } else {
        /* Copy names from Device Tree */
        for (i = 0; i < g_ctx->led_count; i++) {
            g_ctx->led_names[i] = names[i];
        }
        dev_info(dev, "Using Device Tree LED names\n");
    }
    
    /* Log configuration */
    for (i = 0; i < g_ctx->led_count; i++) {
        dev_info(dev, "LED %d: %s\n", i, g_ctx->led_names[i]);
    }
    
    return 0;
}

/**
 * @brief Platform driver probe function
 */
static int gpio_led_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    int ret;
    
    dev_info(dev, "GPIO LED descriptor driver probe\n");
    
    /* Allocate context */
    g_ctx = devm_kzalloc(dev, sizeof(*g_ctx), GFP_KERNEL);
    if (!g_ctx) {
        dev_err(dev, "Failed to allocate context\n");
        return -ENOMEM;
    }
    
    g_ctx->pdev = pdev;
    platform_set_drvdata(pdev, g_ctx);
    
    /* Parse Device Tree */
    ret = parse_dt_config(pdev);
    if (ret) {
        dev_err(dev, "Device Tree parsing failed: %d\n", ret);
        return ret;
    }
    
    /* Initialize synchronization */
    mutex_init(&g_ctx->lock);
    g_ctx->current_led = 0;
    
    /* Allocate character device number */
    ret = alloc_chrdev_region(&g_ctx->dev_num, 0, 1, DEVICE_NAME);
    if (ret) {
        dev_err(dev, "Failed to allocate chrdev region: %d\n", ret);
        return ret;
    }
    
    /* Initialize and add character device */
    cdev_init(&g_ctx->cdev, &gpio_led_fops);
    ret = cdev_add(&g_ctx->cdev, g_ctx->dev_num, 1);
    if (ret) {
        dev_err(dev, "Failed to add cdev: %d\n", ret);
        goto err_chrdev;
    }
    
    /* Create device class */
    g_ctx->dev_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(g_ctx->dev_class)) {
        ret = PTR_ERR(g_ctx->dev_class);
        dev_err(dev, "Failed to create class: %d\n", ret);
        goto err_cdev;
    }
    
    /* Create device node */
    g_ctx->dev_node = device_create(g_ctx->dev_class, NULL, 
                                   g_ctx->dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(g_ctx->dev_node)) {
        ret = PTR_ERR(g_ctx->dev_node);
        dev_err(dev, "Failed to create device: %d\n", ret);
        goto err_class;
    }
    
    dev_info(dev, "Driver loaded successfully\n");
    dev_info(dev, "Device created: /dev/%s\n", DEVICE_NAME);
    dev_info(dev, "LEDs configured: %d\n", g_ctx->led_count);
    
    return 0;
    
err_class:
    class_destroy(g_ctx->dev_class);
err_cdev:
    cdev_del(&g_ctx->cdev);
err_chrdev:
    unregister_chrdev_region(g_ctx->dev_num, 1);
    return ret;
}

/**
 * @brief Platform driver remove function - C90 Compliant
 */
static int gpio_led_remove(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    int i;  /* C90: Declare variable at top */
    
    dev_info(dev, "GPIO LED driver removal\n");
    
    /* Turn off all LEDs - C90 style for loop */
    for (i = 0; i < g_ctx->led_count; i++) {
        led_set_state(i, false);
    }
    
    /* Cleanup character device */
    device_destroy(g_ctx->dev_class, g_ctx->dev_num);
    class_destroy(g_ctx->dev_class);
    cdev_del(&g_ctx->cdev);
    unregister_chrdev_region(g_ctx->dev_num, 1);
    
    dev_info(dev, "Driver removed successfully\n");
    return 0;
}

/* Character Device Operations */

static int gpio_led_open(struct inode *inode, struct file *file)
{
    file->private_data = g_ctx;
    dev_info(&g_ctx->pdev->dev, "Device opened\n");
    return 0;
}

static int gpio_led_release(struct inode *inode, struct file *file)
{
    dev_info(&g_ctx->pdev->dev, "Device closed\n");
    return 0;
}

/**
 * @brief Character device read operation - C90 Compliant
 */
static ssize_t gpio_led_read(struct file *file, char __user *buf, 
                            size_t count, loff_t *pos)
{
    char status[512];
    int len = 0;
    int i;        /* C90: Declare variables at top */
    bool state;
    
    if (*pos > 0) return 0;
    
    mutex_lock(&g_ctx->lock);
    
    len += snprintf(status + len, sizeof(status) - len,
                   "Current LED: %d (%s)\n", g_ctx->current_led,
                   g_ctx->led_names[g_ctx->current_led]);
    
    len += snprintf(status + len, sizeof(status) - len, "LED States:\n");
    
    /* C90 style for loop */
    for (i = 0; i < g_ctx->led_count; i++) {
        state = led_get_state(i);
        len += snprintf(status + len, sizeof(status) - len,
                       "  %d (%s): %s\n", i, g_ctx->led_names[i],
                       state ? "ON" : "OFF");
    }
    
    mutex_unlock(&g_ctx->lock);
    
    if (count < len) return -EINVAL;
    
    if (copy_to_user(buf, status, len)) return -EFAULT;
    
    *pos = len;
    return len;
}

/**
 * @brief Character device write operation - C90 Compliant
 */
static ssize_t gpio_led_write(struct file *file, const char __user *buf, 
                             size_t count, loff_t *pos)
{
    char cmd;
    bool state;  /* C90: Declare variables at top */
    
    if (count < 1) return -EINVAL;
    
    if (copy_from_user(&cmd, buf, 1)) return -EFAULT;
    
    mutex_lock(&g_ctx->lock);
    
    switch (cmd) {
    case '1':
    case 'H':
    case 'h':
        led_set_state(g_ctx->current_led, true);
        break;
    case '0':
    case 'L':
    case 'l':
        led_set_state(g_ctx->current_led, false);
        break;
    case 'T':
    case 't':
        /* C90: No mixed declarations */
        state = led_get_state(g_ctx->current_led);
        led_set_state(g_ctx->current_led, !state);
        break;
    default:
        mutex_unlock(&g_ctx->lock);
        return -EINVAL;
    }
    
    mutex_unlock(&g_ctx->lock);
    return count;
}

/**
 * @brief Character device IOCTL operation - C90 Compliant
 */
static long gpio_led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    int led_idx, state, state_int, current_led_idx, count_val, all_states;  /* C90: All variables at top */
    int i;
    bool led_state;
    
    if (_IOC_TYPE(cmd) != GPIO_LED_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > GPIO_LED_MAX_CMD) return -ENOTTY;
    
    switch (cmd) {
    case GPIO_LED_SELECT:
        if (copy_from_user(&led_idx, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        
        mutex_lock(&g_ctx->lock);
        if (led_idx >= 0 && led_idx < g_ctx->led_count) {
            g_ctx->current_led = led_idx;
            dev_info(&g_ctx->pdev->dev, "Selected LED %d (%s)\n", 
                    led_idx, g_ctx->led_names[led_idx]);
        } else {
            ret = -EINVAL;
        }
        mutex_unlock(&g_ctx->lock);
        break;
        
    case GPIO_LED_SET_STATE:
        if (copy_from_user(&state, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        
        mutex_lock(&g_ctx->lock);
        led_set_state(g_ctx->current_led, state ? true : false);
        mutex_unlock(&g_ctx->lock);
        break;
        
    case GPIO_LED_GET_STATE:
        mutex_lock(&g_ctx->lock);
        led_state = led_get_state(g_ctx->current_led);
        state_int = led_state ? 1 : 0;
        mutex_unlock(&g_ctx->lock);
        
        if (copy_to_user((int __user *)arg, &state_int, sizeof(int)))
            return -EFAULT;
        break;
        
    case GPIO_LED_TOGGLE:
        mutex_lock(&g_ctx->lock);
        led_state = led_get_state(g_ctx->current_led);
        led_set_state(g_ctx->current_led, !led_state);
        mutex_unlock(&g_ctx->lock);
        break;
        
    case GPIO_LED_GET_CURRENT:
        mutex_lock(&g_ctx->lock);
        current_led_idx = g_ctx->current_led;  /* Renamed from 'current' to avoid kernel macro conflict */
        mutex_unlock(&g_ctx->lock);
        
        if (copy_to_user((int __user *)arg, &current_led_idx, sizeof(int)))
            return -EFAULT;
        break;
        
    case GPIO_LED_GET_COUNT:
        mutex_lock(&g_ctx->lock);
        count_val = g_ctx->led_count;
        mutex_unlock(&g_ctx->lock);
        
        if (copy_to_user((int __user *)arg, &count_val, sizeof(int)))
            return -EFAULT;
        break;
        
    case GPIO_LED_SET_ALL:
        if (copy_from_user(&state, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        
        mutex_lock(&g_ctx->lock);
        /* C90 style for loop */
        for (i = 0; i < g_ctx->led_count; i++) {
            led_set_state(i, state ? true : false);
        }
        mutex_unlock(&g_ctx->lock);
        break;
        
    case GPIO_LED_GET_ALL:
        all_states = 0;
        mutex_lock(&g_ctx->lock);
        /* C90 style for loop */
        for (i = 0; i < g_ctx->led_count; i++) {
            if (led_get_state(i)) {
                all_states |= (1 << i);
            }
        }
        mutex_unlock(&g_ctx->lock);
        
        if (copy_to_user((int __user *)arg, &all_states, sizeof(int)))
            return -EFAULT;
        break;
        
    default:
        ret = -ENOTTY;
        break;
    }
    
    return ret;
}

/**
 * @brief Platform driver registration using modern macro
 * 
 * module_platform_driver() automatically generates:
 * - module_init() function that calls platform_driver_register()
 * - module_exit() function that calls platform_driver_unregister()
 * 
 * This is the recommended approach for platform drivers.
 */
module_platform_driver(gpio_led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TungNHS");
MODULE_DESCRIPTION("GPIO LED Driver using Descriptor API");
MODULE_VERSION("1.0");