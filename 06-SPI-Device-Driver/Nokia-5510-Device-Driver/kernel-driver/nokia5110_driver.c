/**
 * @file nokia5110_driver.c
 * @brief Nokia 5110 LCD Display Driver
 * @author TungNHS
 * @version 1.0
 * 
 */

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>


#include "nokia5110_driver.h"

/* Global driver instance*/
static struct nokia5110_device_context *global_nokia_device = NULL;

/* Function prototypes */
static int nokia5110_spi_probe_callback(struct spi_device *spi_device);
static int nokia5110_spi_remove_callback(struct spi_device *spi_device);
static int nokia5110_char_device_open(struct inode *inode_ptr, struct file *file_ptr);
static int nokia5110_char_device_release(struct inode *inode_ptr, struct file *file_ptr);
static ssize_t nokia5110_char_device_write(struct file *file_ptr, const char __user *user_buffer, 
                                            size_t write_count, loff_t *file_position);
static ssize_t nokia5110_char_device_read(struct file *file_ptr, char __user *user_buffer, 
                                            size_t read_count, loff_t *file_position);

/**
 * @brief 5x7 font table for basic characters
 * Font data for displaying alphanumeric characters on LCD
 */
static const uint8_t font_table_5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* ' ' (space) */
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* '0' */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* '1' */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* '2' */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* '3' */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* '4' */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* '5' */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* '6' */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* '7' */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* '8' */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* '9' */
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, /* 'A' */
    {0x7F, 0x49, 0x49, 0x49, 0x36}, /* 'B' */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* 'C' */
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, /* 'D' */
    {0x7F, 0x49, 0x49, 0x49, 0x41}, /* 'E' */
    {0x7F, 0x09, 0x09, 0x09, 0x01}, /* 'F' */
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, /* 'G' */
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, /* 'H' */
    {0x00, 0x41, 0x7F, 0x41, 0x00}, /* 'I' */
    {0x20, 0x40, 0x41, 0x3F, 0x01}, /* 'J' */
    {0x7F, 0x08, 0x14, 0x22, 0x41}, /* 'K' */
    {0x7F, 0x40, 0x40, 0x40, 0x40}, /* 'L' */
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, /* 'M' */
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, /* 'N' */
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, /* 'O' */
    {0x7F, 0x09, 0x09, 0x09, 0x06}, /* 'P' */
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, /* 'Q' */
    {0x7F, 0x09, 0x19, 0x29, 0x46}, /* 'R' */
    {0x46, 0x49, 0x49, 0x49, 0x31}, /* 'S' */
    {0x01, 0x01, 0x7F, 0x01, 0x01}, /* 'T' */
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, /* 'U' */
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* 'V' */
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, /* 'W' */
    {0x63, 0x14, 0x08, 0x14, 0x63}, /* 'X' */
    {0x07, 0x08, 0x70, 0x08, 0x07}, /* 'Y' */
    {0x61, 0x51, 0x49, 0x45, 0x43}, /* 'Z' */
};

/**
 * @brief Device Tree matching table
 * Table to match device tree compatible strings
 */
static const struct of_device_id nokia5110_device_tree_match_table[] = {
    {.compatible = "simple,nokia5110-lcd"},
    {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, nokia5110_device_tree_match_table);

/**
 * @brief SPI device ID table
 * Table for SPI device identification
 */
static const struct spi_device_id nokia5110_spi_device_id_table[] = {
    {"nokia5110-lcd", 0},
    {/* sentinel */}
};
MODULE_DEVICE_TABLE(spi, nokia5110_spi_device_id_table);

/**
 * @brief File operations structure
 * Define file operations for character device
 */
static const struct file_operations nokia5110_char_device_file_operations = {
    .owner = THIS_MODULE,
    .open = nokia5110_char_device_open,
    .release = nokia5110_char_device_release,
    .write = nokia5110_char_device_write,
    .read = nokia5110_char_device_read,
};

/**
 * @brief SPI driver structure
 * Main SPI driver registration structure
 */
static struct spi_driver nokia5110_spi_driver_instance = {
    .driver = {
        .name = SPI_DRIVER_NAME,
        .of_match_table = nokia5110_device_tree_match_table,
        .owner = THIS_MODULE,
    },
    .probe = nokia5110_spi_probe_callback,
    .remove = nokia5110_spi_remove_callback,
    .owner = THIS_MODULE,
};

/**
 * @brief Send command to Nokia 5110 via SPI
 * @param device_ctx Pointer to device context
 * @param command_byte Commnad byte to send
 * @return 0 on success, negative error code on failure
 */
static int nokia5110_send_spi_command(struct nokia5110_device_context *device_ctx, uint8_t command_byte) {
    int transmission_result;

    /* Set D/C pin LOW for command node */
    gpio_set_value(device_ctx->dc_gpio_pin, GPIO_LOW);

    /* Send command via SPI */
    transmission_result = spi_write(device_ctx->spi_device_ptr, &command_byte, 1);
    if (transmission_result < 0) {
        dev_err(&device_ctx->spi_device_ptr->dev, "Failed to send command 0x%02X, error: %d\n", 
                                                command_byte, transmission_result);
        return transmission_result;
    }

    return 0;
}

/**
 * @brief Send data to Nokia 5110 via SPI
 * @param device_ctx Pointer to device context
 * @param data_byte Data byte to send
 * @return 0 on success, negative error code on failure
 */
static int nokia5110_send_spi_data(struct nokia5110_device_context *device_ctx, uint8_t data_byte) {
    int transmission_result;

    /* Set D/C pin HIGH for data node */
    gpio_set_value(device_ctx->dc_gpio_pin, GPIO_HIGH);

    /* Send data via SPI */
    transmission_result = spi_write(device_ctx->spi_device_ptr, &data_byte, 1);
    if (transmission_result < 0) {
        dev_err(&device_ctx->spi_device_ptr->dev, "Failed to send data 0x%02X, error: %d\n", 
                                                data_byte, transmission_result);
        return transmission_result;
    }

    return 0;
}

/**
 * @brief Initialize Nokia 5110 display hardware
 * @param device_ctx Pointer to device context structure
 * @return 0 on success, negative error code on failure
 */
int nokia5110_initialize_display_hardware(struct nokia5110_device_context *device_ctx) {
    dev_info(&device_ctx->spi_device_ptr->dev, "Initializing Nokia 5110 display hardware\n");

    /* Resquest and configure GPIO pins */
    if (gpio_request(device_ctx->reset_gpio_pin, "nokia5110-reset") < 0) {
        dev_err(&device_ctx->spi_device_ptr->dev, "Failed to request reset GPIO\n");
        return -EIO;
    }

    if (gpio_request(device_ctx->dc_gpio_pin, "nokia5110-dc") < 0) {
        dev_err(&device_ctx->spi_device_ptr->dev, "Failed to request DC GPIO\n");
        gpio_free(device_ctx->reset_gpio_pin);
        return -EIO;
    }
    
    /* Configure GPIO as outputs */
    gpio_direction_output(device_ctx->reset_gpio_pin, GPIO_HIGH);
    gpio_direction_output(device_ctx->dc_gpio_pin, GPIO_LOW);

    /* Hardware reset sequence */
    gpio_set_value(device_ctx->reset_gpio_pin, GPIO_LOW);
    msleep(10);
    gpio_set_value(device_ctx->dc_gpio_pin, GPIO_HIGH);
    msleep(10);

    /* LCD initialization sequence */
    nokia5110_send_spi_command(device_ctx, LCD_CMD_EXTENDED_INSTR);     // Extended instruction set
    nokia5110_send_spi_command(device_ctx, LCD_CMD_CONTRAST);           // Set contrast
    nokia5110_send_spi_command(device_ctx, LCD_CMD_TEMP_COEFF);         // Temperature coefficient
    nokia5110_send_spi_command(device_ctx, LCD_CMD_BIAS_SYSTEM);        // Bias system
    nokia5110_send_spi_command(device_ctx, LCD_CMD_FUNCTION_SET);       // Function set
    nokia5110_send_spi_command(device_ctx, LCD_CMD_DISPLAY_CONTROL);    // Display control

    /* Clear screen */
    nokia5110_clear_display_screen(device_ctx);

    /* Set initial device state */
    device_ctx->is_display_enabled = true;
    device_ctx->display_contrast_level = 0xB1;
    device_ctx->current_cursor_x = 0;
    device_ctx->current_cursor_y = 0;

    dev_info(&device_ctx->spi_device_ptr->dev, "Nokia 5110 display hardware initialized successfully\n");
    return 0;
}

/**
 * @brief Clear entrie display screen
 * @param device_ctx Pointer to device context structure
 * @return 0 on success, negative error code on failure
 */
int nokia5110_clear_display_screen(struct nokia5110_device_context *device_ctx) {
    int bank_index, column_index;

    /* Clear all banks and columns */
    for (bank_index = 0; bank_index < DISPLAY_TOTAL_BANKS; bank_index++) {
        /* Set cursor position to start of bank */
        nokia5110_send_spi_command(device_ctx, LCD_CMD_SET_Y_ADDRESS | bank_index);
        nokia5110_send_spi_command(device_ctx, LCD_CMD_SET_X_ADDRESS | 0);

        /* Clear all columns in this bank */
        for (column_index = 0; column_index < DISPLAY_WIDTH_PIXELS; column_index++) {\
            nokia5110_send_spi_data(device_ctx, 0x00);
        }
    }

    /* Reset cursor position */
    device_ctx->current_cursor_x = 0;
    device_ctx->current_cursor_y = 0;

    return 0;
}

/**
 * @brief Set cursor position on display
 * @param device_ctx Pointer to device context
 * @param x_pos X position (0-83)
 * @param y_pos Y position (0-5)
 * @return 0 on success, negative error code on failure
 */
static int nokia5110_set_cursor_position(struct nokia5110_device_context *device_ctx, uint8_t x_pos, uint8_t y_pos) {
    if (x_pos >= DISPLAY_WIDTH_PIXELS || y_pos >= DISPLAY_TOTAL_BANKS) {
        return -EINVAL;    
    }

    /* Set Y address (bank) */
    nokia5110_send_spi_command(device_ctx, LCD_CMD_SET_Y_ADDRESS | y_pos);

    /* Set X addesss (column) */
    nokia5110_send_spi_data(device_ctx, LCD_CMD_SET_X_ADDRESS | x_pos);

    device_ctx->current_cursor_x = 0;
    device_ctx->current_cursor_y = 0;

    return 0;
}

/**
 * @brief Write single character to display
 * @param device_ctx Pointer to device context
 * @param character Character to display
 * @return 0 on success, negative error code on failure
 */
static int nokia5110_write_single_character(struct nokia5110_device_context *device_ctx, char character)
{
    int font_byte_index;
    const uint8_t *font_data_ptr;
    
    /* Handle newline character */
    if (character == '\n') {
        device_ctx->current_cursor_y++;
        device_ctx->current_cursor_x = 0;
        if (device_ctx->current_cursor_y >= DISPLAY_TOTAL_BANKS) {
            device_ctx->current_cursor_y = 0;   // Wrap to top
        }
        nokia5110_set_cursor_position(device_ctx, device_ctx->current_cursor_x, device_ctx->current_cursor_y);
        return 0;
    }
    
    /* Handle line wrap */
    if (device_ctx->current_cursor_x >= DISPLAY_WIDTH_PIXELS - FONT_CHAR_WIDTH) {
        device_ctx->current_cursor_y++;
        device_ctx->current_cursor_x = 0;
        if (device_ctx->current_cursor_y >= DISPLAY_TOTAL_BANKS) {
            device_ctx->current_cursor_y = 0;
        }
        nokia5110_set_cursor_position(device_ctx, device_ctx->current_cursor_x, device_ctx->current_cursor_y);
    }
    
    /* Get font data - simple character mapping */
    if (character >= '0' && character <= '9') {
        font_data_ptr = font_table_5x7[character - '0' + 1]; /* +1 because space is at index 0 */
    } else if (character >= 'A' && character <= 'Z') {
        font_data_ptr = font_table_5x7[character - 'A' + 11];
    } else if (character >= 'a' && character <= 'z') {
        font_data_ptr = font_table_5x7[character - 'a' + 11]; /* Use uppercase font */
    } else {
        font_data_ptr = font_table_5x7[0]; /* Space for unknown characters */
    }
    
    /* Send font data to display */
    for (font_byte_index = 0; font_byte_index < 5; font_byte_index++) {
        nokia5110_send_spi_data(device_ctx, font_data_ptr[font_byte_index]);
    }
    
    /* Add space between characters */
    nokia5110_send_spi_data(device_ctx, 0x00);
    
    device_ctx->current_cursor_x += FONT_CHAR_WIDTH;
    
    return 0;
}

/**
 * @brief Write text string to display
 * @param device_ctx Pointer to device context structure
 * @param text_string Null-terminated text string to display
 * @return 0 on success, negative error code on failure
 */
int nokia5110_write_text_to_display(struct nokia5110_device_context *device_ctx, 
                                    const char *text_string)
{
    while (*text_string) {
        nokia5110_write_single_character(device_ctx, *text_string++);
    }
    return 0;
}

/**
 * @brief Create character device file node
 * @param device_ctx Pointer to device context
 * @return 0 on success, negative error code on failure
 */
static int nokia5110_create_character_device(struct nokia5110_device_context *device_ctx) {
    int result = 0;
    
    dev_info(&device_ctx->spi_device_ptr->dev, "Creating Nokia 5110 character device\n");

    /* Allocate character device region */
    result = alloc_chrdev_region(&device_ctx->char_device_number, 0, 1, DEVICE_NAME);
    if (result < 0) {
        dev_err(&device_ctx->spi_device_ptr->dev, "Failed to allocate character device region: %d\n", result);
        goto allocation_failed;
    }

    dev_info(&device_ctx->spi_device_ptr->dev, "Character device allocated: major=%d, minor=%d\n", 
                                                MAJOR(device_ctx->char_device_number), 
                                                MINOR(device_ctx->char_device_number));
    
    /* Create device class */
    device_ctx->char_device_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);
    if (IS_ERR(device_ctx->char_device_class)) {
        result = PTR_ERR(device_ctx->char_device_class);
        dev_err(&device_ctx->spi_device_ptr->dev, "Failed to create device class: %d\n", result);
        goto class_creation_failed;
    }

    /* Create device node */
    device_ctx->char_device_node = device_create(device_ctx->char_device_class, NULL,
                                                device_ctx->char_device_number, NULL, 
                                                DEVICE_NAME);                                   
    if (IS_ERR(device_ctx->char_device_node)) {
        result = PTR_ERR(device_ctx->char_device_node);
        dev_err(&device_ctx->spi_device_ptr->dev, "Failed to create device node: %d\n", result);
        goto device_creation_failed;
    }

    /* Initialize and add character device */
    cdev_init(&device_ctx->char_device_cdev, &nokia5110_char_device_file_operations);
    device_ctx->char_device_cdev.owner = THIS_MODULE;

    result = cdev_add(&device_ctx->char_device_cdev, device_ctx->char_device_number, 1);
    if (result) {
        dev_err(&device_ctx->spi_device_ptr->dev, "Failed to add character device: %d\n", result);
        goto cdev_add_failed;
    }

    dev_info(&device_ctx->spi_device_ptr->dev, "Character device created successfully: /dev/%s\n", DEVICE_NAME);
    return 0;

    /* Error cleanup */
cdev_add_failed:
    device_destroy(device_ctx->char_device_class, device_ctx->char_device_number);
device_creation_failed:
    class_destroy(device_ctx->char_device_class);
class_creation_failed:
    unregister_chrdev_region(device_ctx->char_device_number, 1);
allocation_failed:
    return result;
}

/**
 * @brief SPI probe callback function
 * @param spi_device Pointer to SPI device structure
 * @return 0 on success, negative error code on failure
 */
static int nokia5110_spi_probe_callback(struct spi_device *spi_device) {
    struct nokia5110_device_context *device_ctx;
    struct device_node *device_tree_node = spi_device->dev.of_node;
    int result;

    dev_info(&spi_device->dev, "Nokia 5110 SPI probe started");

    /* Allocate device context structure */
    device_ctx = devm_kzalloc(&spi_device->dev, sizeof(*device_ctx), GFP_KERNEL);
    if (!device_ctx) {
        dev_err(&spi_device->dev, "Failed to allocate device context memory\n");
        return -ENOMEM;
    }

    /* Initialize device context */
    device_ctx->spi_device_ptr = spi_device;
    spi_set_drvdata(spi_device, device_ctx);

    /* Get GPIO pins from Device tree */
    device_ctx->reset_gpio_pin = of_get_named_gpio(device_tree_node, "reset-gpios", 0);
    if (device_ctx->reset_gpio_pin < 0) {
        dev_err(&spi_device->dev, "Failed to get reset GPIO from Device tree\n");
        return device_ctx->reset_gpio_pin;
    }

    device_ctx->dc_gpio_pin = of_get_named_gpio(device_tree_node, "dc-gpios", 0);
    if (device_ctx->dc_gpio_pin < 0) {
        dev_err(&spi_device->dev, "Failed to get reset GPIO from Device tree\n");
        return device_ctx->dc_gpio_pin;
    }

    dev_info(&spi_device->dev, "GPIO pins: reset=%d, dc=%d\n", device_ctx->reset_gpio_pin,
                                                            device_ctx->dc_gpio_pin);

    /* Initialize display hardware */
    result = nokia5110_initialize_display_hardware(device_ctx);
    if (result) {
        dev_err(&spi_device->dev, "Failed to initialize display hardware: %d\n", result);
        return result;
    }

    /* Set cursor and display demo message */
    nokia5110_set_cursor_position(device_ctx, 0, 0);
    nokia5110_write_text_to_display(device_ctx, "NOKIA 5110\nREADY");

    /* Create character device */
    result = nokia5110_create_character_device(device_ctx);
    if (result) {
        dev_err(&spi_device->dev, "Failed to create character device: %d\n", result);
        return result;
    }

    /* Set global device instance */
    global_nokia_device = device_ctx;

    dev_info(&spi_device->dev, "Nokia 5110 probe completed successfully\n");
    return 0;
};

/**
 * @brief SPI remove callback function
 * @param spi_device Pointer to SPI device structure
 * @return 0 on success, negative error code on failure
 */
static int nokia5110_spi_remove_callback(struct spi_device *spi_device) {
    struct nokia5110_device_context *device_ctx = spi_get_drvdata(spi_device);

    dev_info(&spi_device->dev, "Nokia 5110 SPI remove started\n");

    /* Display goobye message */
    nokia5110_write_text_to_display(device_ctx, "GOODBYE!\nShutdown...");
    msleep(1000);

    /* Clear display */
    nokia5110_clear_display_screen(device_ctx);

    /* Clean up character device */
    cdev_del(&device_ctx->char_device_cdev);
    device_destroy(device_ctx->char_device_class, device_ctx->char_device_number);
    class_destroy(device_ctx->char_device_class);
    unregister_chrdev_region(device_ctx->char_device_number, 1);

    /* Free GPIO pins */
    gpio_free(device_ctx->reset_gpio_pin);
    gpio_free(device_ctx->dc_gpio_pin);

        /* Clear global reference */
    global_nokia5110_device = NULL;
    
    dev_info(&spi_device->dev, "Nokia 5110 SPI remove completed\n");
    return 0;
}

/**
 * @brief Register SPI driver using modern marco
 * module_spi_driver() automatically generates:
 *  - module_init() function that calls spi_register_driver()
 *  - module_exit() function that calls spi_unregister_driver()
 */
module_spi_driver(nokia5110_spi_driver_instance);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TungNHS");
MODULE_DESCRIPTION("Nokia 5110 LCD Display SPI Driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("spi:nokia5110-lcd");