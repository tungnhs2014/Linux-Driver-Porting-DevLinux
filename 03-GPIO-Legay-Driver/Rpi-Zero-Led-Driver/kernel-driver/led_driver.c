/**
 * @file led_driver.c
 * @brief LED character device driver implementation
 * Provides /dev/led interface for GPIO LED control via direct register access
 */

 #include <linux/module.h>  /* For MODULE_marcos */
 #include <linux/kernel.h>  /* Forkernel logging functions */
 #include <linux/fs.h>      /* For file_operations, register_chrdev_region */
 #include <linux/cdev.h>    /* For character device functions */
 #include <linux/device.h>  /* For device_create/class_create */
 #include <linux/uaccess.h> /* For copy_to/from_user */
 #include <linux/io.h>      /* For ioremap, iounmap */
 #include <linux/slab.h>    /* For kzalloc/kfree */
 #include "led_driver.h"

 /**
  * @brief LED driver device structure
  * Encapsulates all driver-related data
  */
 struct led_driver_data {
    dev_t dev_num;                  /* Device number (major + minor) */
    struct class *device_class;     /* Device class for sysfs */
    struct device *device;          /* Device structure */
    struct cdev char_device;        /* Character device structure */
    void __iomem *gpio_base;        /* Mapped GPIO register base */
    int gpio_pin;                   /* GPIO pin number for LED */
    bool current_state;             /* Current LED state */
    struct mutex state_mutex;       /* Protects state changes */
 };
 
 /* Driver instance */
 static struct led_driver_data *led_driver;

 /**
  * @brief Configure GPIO pin as output
  * @param drv Driver data structure
  */
 static void gpio_configure_output(struct led_driver_data *drv, int gpio_pin) {
    uint32_t reg_value;

    /* Calculate which FESEL register and bit position within that register */
    uint32_t fsel_reg = GPIO_FSEL0_OFFSET+ ((gpio_pin / 10) * 4);
    uint32_t bit_offset = (gpio_pin % 10) * 3;
    
    /* Read current value */
    reg_value = readl(drv->gpio_base + fsel_reg);

    /* Clear 3 bits */
    reg_value &= ~(0x7 << bit_offset);

    /* Set as output */
    reg_value |= (GPIO_FUNCTION_OUTPUT << bit_offset);

    /* Write updated value */
    writel(reg_value, drv->gpio_base + fsel_reg);

    pr_info("LED: GPIO%d configured as output\n", gpio_pin);
 };

 /**
  * @brief Set GPIO pin to high level
  * @param drv Driver data structure
  * @param gpio_pin GPIO pin number
  */
 static void gpio_set_high(struct led_driver_data *drv, int gpio_pin) {
    writel(1 << gpio_pin, drv->gpio_base + GPIO_SET0_OFFSET);
 }

 /**
  * @brief Set GPIO pin to low level
  * @param drv Driver data structure
  * @param gpio_pin GPIO pin number
  */
 static void gpio_set_low(struct led_driver_data *drv, int gpio_pin) {
    writel(1 << gpio_pin, drv->gpio_base + GPIO_CLR0_OFFSET);
 }
 
 /**
  * @brief Read GPIO pin level
  * @param drv Driver data structure
  * @param gpio_pin GPIO pin number
  * @return true if high, false if low
  */
 static bool gpio_read_level(struct led_driver_data *drv, int gpio_pin) {
    uint32_t reg_value = readl(drv->gpio_base + GPIO_LEV0_OFFSET);
    return (reg_value >> gpio_pin) & 1; 
 }

 /**
  * @brief Device open operation 
  */
 static int led_device_open(struct inode *inode, struct file *file)
 {
    pr_info("LED: Device opened\n");
    return 0;
 }

 /**
  * @brief Device release operation
  */
 static int led_device_release(struct inode *inode, struct file *file) {
    pr_info("LED: Device closed\n");
    return 0;
 }

 /**
  * @brief Device read opeation
  * @return Current LED state as string
  */
 static ssize_t led_device_read(struct file *file, char __user *buffer, size_t len, loff_t *offset) {
    char status_msg[64];
    int msg_len;

    if (*offset > 0) {
        return 0; /* EOF */
    }

    mutex_lock(&led_driver->state_mutex);

    /* Update state from hardware */
    led_driver->current_state = gpio_read_level(led_driver, led_driver->gpio_pin);

    msg_len = snprintf(status_msg, sizeof(status_msg), "LED: %s (GPIO%d)\n", 
                        led_driver->current_state ? "ON" : "OFF",
                        led_driver->gpio_pin);
    
    mutex_unlock(&led_driver->state_mutex);

    if (len < msg_len) {
        return -EINVAL;
    }

    if (copy_to_user(buffer, status_msg, msg_len)) {
        return -EFAULT;
    }

    pr_info("LED: Status read - LED is %s\n", led_driver->current_state ? "ON" : "OFF");

    *offset += msg_len;
    return msg_len;
 }
 
 /**
  * @brief Device write operation
  * @return Accepts '0' (off) or '1' (on) commands
  */
 static ssize_t led_device_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
    char cmd;
    
    if (len < 1) {
        return -EINVAL;
    }

    if (copy_from_user(&cmd, buffer, 1)) {
        return -EFAULT;
    }

    mutex_lock(&led_driver->state_mutex);

    switch (cmd)
    {
    case LED_CMD_ON:
        gpio_set_high(led_driver, led_driver->gpio_pin);
        led_driver->current_state = true;
        pr_info("LED GPIO %d turned ON\n", led_driver->gpio_pin);
        break;

    case LED_CMD_OFF:
        gpio_set_low(led_driver, led_driver->gpio_pin);
        led_driver->current_state = false;
        pr_info("LED GPIO %d turned ON\n", led_driver->gpio_pin);
        break;

    default:
        mutex_unlock(&led_driver->state_mutex);
        pr_warn("LED: Invalid command '%c'\n", cmd);
        return -EINVAL;
        break;
    }

    mutex_unlock(&led_driver->state_mutex);
    return len;
 }


 /* File operations structure */
 static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_device_open,
    .read = led_device_read,
    .write = led_device_write,
    .release = led_device_release,
 };

 /**
  * @brief Module initialization
  */
 static int __init led_driver_init(void) {
    int ret;

    pr_info("LED: Initializing driver for PI Zero W\n");

    /* Allocate driver structure */
    led_driver = kzalloc(sizeof(*led_driver), GFP_KERNEL);
    if (!led_driver) {
        pr_err("LED: Failed to allocate driver structure\n");
        return -ENOMEM;
    }

    /* Initialize driver data */
    led_driver->gpio_pin = DEFAULT_LED_GPIO;
    led_driver->current_state = false;
    mutex_init(&led_driver->state_mutex);

    /* Map GPIO registers */
    led_driver->gpio_base = ioremap(BCM2835_GPIO_BASE, GPIO_REGISTER_SIZE);
    if (!led_driver->gpio_base) {
        pr_err("LED: Failed to map GPIO registers\n");
        ret = -ENOMEM;
        goto cleanup_memory;
    }
    pr_info("LED: GPIO registers mapped successfully\n");

    /* Configure GPIO as output and set initial state */
    gpio_configure_output(led_driver, led_driver->gpio_pin);
    gpio_set_low(led_driver, led_driver->gpio_pin);
    
    /* Allocate device number */
    ret = alloc_chrdev_region(&led_driver->dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("LED: Failed to allocate device number\n");
        goto cleanup_gpio;
    }
    
    /* Initialize and add character device */
    cdev_init(&led_driver->char_device, &led_fops);
    led_driver->char_device.owner = THIS_MODULE;
    
    ret = cdev_add(&led_driver->char_device, led_driver->dev_num, 1);
    if (ret < 0) {
        pr_err("LED: Failed to add character device\n");
        goto cleanup_chrdev_region;
    }
    
    /* Create device class */
    led_driver->device_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(led_driver->device_class)) {
        pr_err("LED: Failed to create device class\n");
        ret = PTR_ERR(led_driver->device_class);
        goto cleanup_cdev;
    }
    
    /* Create device */
    led_driver->device = device_create(led_driver->device_class, NULL, 
                                      led_driver->dev_num, 
                                      NULL, DEVICE_NAME);
    if (IS_ERR(led_driver->device)) {
        pr_err("LED: Failed to create device\n");
        ret = PTR_ERR(led_driver->device);
        goto cleanup_class;
    }
    
    pr_info("LED: Driver loaded - /dev/%s created (major:%d, minor:%d, GPIO%d)\n", 
            DEVICE_NAME, MAJOR(led_driver->dev_num), MINOR(led_driver->dev_num), 
            led_driver->gpio_pin);
    
    return 0;
    
    /* Error cleanup */
cleanup_class:
    class_destroy(led_driver->device_class);
cleanup_cdev:
    cdev_del(&led_driver->char_device);
cleanup_chrdev_region:
    unregister_chrdev_region(led_driver->dev_num, 1);
cleanup_gpio:
    iounmap(led_driver->gpio_base);
cleanup_memory:
    kfree(led_driver);
    pr_err("LED: Driver initialization failed\n");
    return ret;
 }

 static void __exit led_driver_exit(void) {
    if (!led_driver)
        return;
    
    pr_info("LED: Unloading driver\n");
    
    /* Turn off LED */
    mutex_lock(&led_driver->state_mutex);
    gpio_set_low(led_driver, led_driver->gpio_pin);
    led_driver->current_state = false;
    mutex_unlock(&led_driver->state_mutex);
    
    /* Cleanup in reverse order */
    device_destroy(led_driver->device_class, led_driver->dev_num);
    class_destroy(led_driver->device_class);
    cdev_del(&led_driver->char_device);
    unregister_chrdev_region(led_driver->dev_num, 1);
    iounmap(led_driver->gpio_base);
    kfree(led_driver);
    
    pr_info("LED: Driver unloaded successfully\n");
 }

 module_init(led_driver_init);
 module_exit(led_driver_exit);
 
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("TungNHS");
 MODULE_DESCRIPTION("LED Character Device Driver for Raspberry Pi Zero W");
 MODULE_VERSION("1.0");