/**
 * @file ssd1306_driver.c
 * @brief SSD1306 OLED Display Driver
 * @author TungNHS
 * @version 1.0
 * 
 * I2C driver for SSD1306 OLED display.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/cdev.h>

#include "ssd1306_driver.h"

/* Global driver instance */
static struct ssd1306_device_context *global_ssd1306_device = NULL;

/* Function prototypes */
static int ssd1306_i2c_probe_callback(struct i2c_client *client, 
                                      const struct i2c_device_id *device_id);
static int ssd1306_i2c_remove_callback(struct i2c_client *client);
static int ssd1306_char_device_open(struct inode *inode_ptr, struct file *file_ptr);
static int ssd1306_char_device_release(struct inode *inode_ptr, struct file *file_ptr);
static ssize_t ssd1306_char_device_write(struct file *file_ptr, const char __user *user_buffer, 
                                         size_t write_count, loff_t *file_position);
static ssize_t ssd1306_char_device_read(struct file *file_ptr, char __user *user_buffer, 
                                        size_t read_count, loff_t *file_position);


/**
 * @brief Device tree matching table
 * Table to match device tree compatible strings
 */
static const struct of_device_id ssd1306_device_tree_match_table[] = {
    {.compatible = "simple, ssd1306-oled"},
    {}
};
MODULE_DEVICE_TABLE(of, ssd1306_device_tree_match_table);

/**
 * @brief I2C device ID table
 * Table for I2C device identification
 */
static const struct i2c_device_id ssd1306_i2c_device_id_table[] = {
    { "ssd1306-oled", 0 },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, ssd1306_i2c_device_id_table);

/**
 * @brief File operations structure
 * Defines file operations for character device
 */
static const struct file_operations ssd1306_char_device_file_operations = {
    .owner = THIS_MODULE;
    .open = ssd1306_char_device_open,
    .release = ssd1306_char_device_release,
    .write = ssd1306_char_device_write,
    .read = ssd1306_char_device_read,
};

/**
 * @brief I2C driver structure
 * Main I2C driver registration structure
 */
static struct i2c_driver ssd1306_i2c_driver_instance = {
    .probe = ssd1306_i2c_probe_callback,
    .remove = ssd1306_i2c_remove_callback,
    .id_table = ssd1306_i2c_device_id_table,
    .driver = {
        .name = I2C_DRIVER_NAME,
        .of_match_table = ssd1306_device_tree_match_table,
        .owner = THIS_MODULE,
    }
};

/**
 * @brief Register I2C driver using modern macro
 * 
 * module_i2c_driver() automatically generates:
 * - module_init() function that calls i2c_add_driver()
 * - module_exit() function that calls i2c_del_driver()
 */
module_i2c_driver(ssd1306_i2c_driver_instance);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TungNHS");
MODULE_DESCRIPTION("SSD1306 OLED Display I2C Driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("i2c:ssd1306-oled");