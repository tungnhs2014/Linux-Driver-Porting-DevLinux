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

/* Global driver instance for educational purposes */
static struct ssd1306_device_context *global_ssd1306_device = NULL;

/* Function prototypes */
static int ssd1306_i2c_probe_callback(struct i2c_client *client, const struct i2c_device_id *device_id);
static int ssd1306_i2c_remove_callback(struct i2c_client *client);
static int ssd1306_char_device_open(struct inode *inode_ptr, struct file *file_ptr);
static int ssd1306_char_device_release(struct inode *inode_ptr, struct file *file_ptr);
static ssize_t ssd1306_char_device_write(struct file *file_ptr, const char __user *user_buffer, 
                                        size_t write_count, loff_t *file_position);
static ssize_t ssd1306_char_device_read(struct file *file_ptr, char __user *user_buffer, 
                                        size_t read_count, loff_t *file_position);

/**
 * @brief Simple 5x8 font table for basic characters
 * Font data for displaying alphanumeric characters on OLED
 */
static const uint8_t font_table_5x8[][5] = {
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
static const struct of_device_id ssd1306_device_tree_match_table[] = {
    { .compatible = "simple,ssd1306-oled" },
    { /* sentinel */ }
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
    .owner = THIS_MODULE,
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
    .driver = {
        .name = I2C_DRIVER_NAME,
        .of_match_table = ssd1306_device_tree_match_table,
        .owner = THIS_MODULE,
    },
    .probe_new = ssd1306_i2c_probe_callback,
    .remove = ssd1306_i2c_remove_callback,
    .id_table = ssd1306_i2c_device_id_table,
};

/**
 * @brief Send command to SSD1306 via I2C
 * @param device_ctx Pointer to device context
 * @param command_byte Command byte to send
 * @return 0 on success, negative error code on failure
 */
static int ssd1306_send_i2c_command(struct ssd1306_device_context *device_ctx, 
                                    uint8_t command_byte)
{
    uint8_t i2c_buffer[2] = {I2C_CMD_PREFIX, command_byte};
    int transmission_result;
    
    transmission_result = i2c_master_send(device_ctx->i2c_client_ptr, i2c_buffer, 2);
    
    if (transmission_result < 0) {
        dev_err(&device_ctx->i2c_client_ptr->dev, 
                "Failed to send command 0x%02X, error: %d\n", 
                command_byte, transmission_result);
        return transmission_result;
    }
    
    return 0;
}

/**
 * @brief Send data to SSD1306 via I2C
 * @param device_ctx Pointer to device context
 * @param data_byte Data byte to send
 * @return 0 on success, negative error code on failure
 */
static int ssd1306_send_i2c_data(struct ssd1306_device_context *device_ctx, 
                                 uint8_t data_byte)
{
    uint8_t i2c_buffer[2] = {I2C_DATA_PREFIX, data_byte};
    int transmission_result;
    
    transmission_result = i2c_master_send(device_ctx->i2c_client_ptr, i2c_buffer, 2);
    
    if (transmission_result < 0) {
        dev_err(&device_ctx->i2c_client_ptr->dev, 
                "Failed to send data 0x%02X, error: %d\n", 
                data_byte, transmission_result);
        return transmission_result;
    }
    
    return 0;
}

/**
 * @brief Initialize SSD1306 display hardware
 * @param device_ctx Pointer to device context structure
 * @return 0 on success, negative error code on failure
 */
int ssd1306_initialize_display_hardware(struct ssd1306_device_context *device_ctx)
{
    dev_info(&device_ctx->i2c_client_ptr->dev, "Initializing SSD1306 display hardware\n");
    
    /* Wait for display to be ready */
    msleep(100);
    
    /* Display OFF during initialization */
    ssd1306_send_i2c_command(device_ctx, SSD1306_CMD_DISPLAY_OFF);
    
    /* Basic initialization sequence - simplified for educational purposes */
    ssd1306_send_i2c_command(device_ctx, 0xD5); /* Set display clock divide ratio */
    ssd1306_send_i2c_command(device_ctx, 0x80); /* Default clock setting */
    
    ssd1306_send_i2c_command(device_ctx, 0xA8); /* Set multiplex ratio */
    ssd1306_send_i2c_command(device_ctx, 0x3F); /* 64 lines */
    
    ssd1306_send_i2c_command(device_ctx, 0xD3); /* Set display offset */
    ssd1306_send_i2c_command(device_ctx, 0x00); /* No offset */
    
    ssd1306_send_i2c_command(device_ctx, 0x40); /* Set start line */
    
    ssd1306_send_i2c_command(device_ctx, 0x8D); /* Charge pump setting */
    ssd1306_send_i2c_command(device_ctx, 0x14); /* Enable charge pump */
    
    ssd1306_send_i2c_command(device_ctx, 0x20); /* Memory addressing mode */
    ssd1306_send_i2c_command(device_ctx, 0x00); /* Horizontal addressing */
    
    ssd1306_send_i2c_command(device_ctx, 0xA1); /* Set segment remap */
    ssd1306_send_i2c_command(device_ctx, 0xC8); /* Set COM scan direction */
    
    ssd1306_send_i2c_command(device_ctx, 0xDA); /* Set COM pins configuration */
    ssd1306_send_i2c_command(device_ctx, 0x12); /* Alternative COM pins */
    
    /* Set contrast */
    ssd1306_send_i2c_command(device_ctx, SSD1306_CMD_SET_CONTRAST);
    ssd1306_send_i2c_command(device_ctx, 0x80); /* Medium contrast */
    
    ssd1306_send_i2c_command(device_ctx, 0xD9); /* Set pre-charge period */
    ssd1306_send_i2c_command(device_ctx, 0xF1); /* Pre-charge setting */
    
    ssd1306_send_i2c_command(device_ctx, 0xDB); /* Set VCOM detect */
    ssd1306_send_i2c_command(device_ctx, 0x20); /* VCOM detect setting */
    
    ssd1306_send_i2c_command(device_ctx, 0xA4); /* Resume to RAM content display */
    ssd1306_send_i2c_command(device_ctx, 0xA6); /* Normal display (not inverted) */
    ssd1306_send_i2c_command(device_ctx, 0x2E); /* Deactivate scroll */
    
    /* Display ON */
    ssd1306_send_i2c_command(device_ctx, SSD1306_CMD_DISPLAY_ON);
    
    /* Clear screen */
    ssd1306_clear_display_screen(device_ctx);
    
    /* Set initial device state */
    device_ctx->is_display_enabled = true;
    device_ctx->display_brightness_level = 128;
    device_ctx->current_cursor_line = 0;
    device_ctx->current_cursor_column = 0;
    
    dev_info(&device_ctx->i2c_client_ptr->dev, 
             "SSD1306 display hardware initialized successfully\n");
    return 0;
}

/**
 * @brief Clear entire display screen
 * @param device_ctx Pointer to device context structure  
 * @return 0 on success, negative error code on failure
 */
int ssd1306_clear_display_screen(struct ssd1306_device_context *device_ctx)
{
    int pixel_index;
    
    /* Set column address range to cover entire display */
    ssd1306_send_i2c_command(device_ctx, SSD1306_CMD_SET_COLUMN_ADDR);
    ssd1306_send_i2c_command(device_ctx, 0x00); /* Column start */
    ssd1306_send_i2c_command(device_ctx, 0x7F); /* Column end (127) */
    
    /* Set page address range to cover entire display */
    ssd1306_send_i2c_command(device_ctx, SSD1306_CMD_SET_PAGE_ADDR);
    ssd1306_send_i2c_command(device_ctx, 0x00); /* Page start */
    ssd1306_send_i2c_command(device_ctx, 0x07); /* Page end (7) */
    
    /* Send zeros to clear all pixels */
    for (pixel_index = 0; pixel_index < (DISPLAY_WIDTH_PIXELS * DISPLAY_TOTAL_PAGES); pixel_index++) {
        ssd1306_send_i2c_data(device_ctx, 0x00);
    }
    
    /* Reset cursor position */
    device_ctx->current_cursor_line = 0;
    device_ctx->current_cursor_column = 0;
    
    return 0;
}

/**
 * @brief Set cursor position on display
 * @param device_ctx Pointer to device context
 * @param line_number Line number (0-7)
 * @param column_number Column number (0-20)
 * @return 0 on success, negative error code on failure
 */
static int ssd1306_set_cursor_position(struct ssd1306_device_context *device_ctx, 
                                       uint8_t line_number, uint8_t column_number)
{
    if (line_number >= MAX_DISPLAY_LINES || column_number >= MAX_CHARS_PER_LINE) {
        return -EINVAL;
    }
    
    /* Set column address */
    ssd1306_send_i2c_command(device_ctx, SSD1306_CMD_SET_COLUMN_ADDR);
    ssd1306_send_i2c_command(device_ctx, column_number * FONT_CHAR_WIDTH);
    ssd1306_send_i2c_command(device_ctx, 127);
    
    /* Set page address */
    ssd1306_send_i2c_command(device_ctx, SSD1306_CMD_SET_PAGE_ADDR);
    ssd1306_send_i2c_command(device_ctx, line_number);
    ssd1306_send_i2c_command(device_ctx, 7);
    
    device_ctx->current_cursor_line = line_number;
    device_ctx->current_cursor_column = column_number;
    
    return 0;
}

/**
 * @brief Write single character to display
 * @param device_ctx Pointer to device context
 * @param character Character to display
 * @return 0 on success, negative error code on failure
 */
static int ssd1306_write_single_character(struct ssd1306_device_context *device_ctx, 
                                          char character)
{
    int font_byte_index;
    const uint8_t *font_data_ptr;
    
    /* Handle newline character */
    if (character == '\n') {
        device_ctx->current_cursor_line++;
        device_ctx->current_cursor_column = 0;
        if (device_ctx->current_cursor_line >= MAX_DISPLAY_LINES) {
            device_ctx->current_cursor_line = 0; /* Wrap to top */
        }
        ssd1306_set_cursor_position(device_ctx, 
                                   device_ctx->current_cursor_line, 
                                   device_ctx->current_cursor_column);
        return 0;
    }
    
    /* Handle line wrap */
    if (device_ctx->current_cursor_column >= MAX_CHARS_PER_LINE) {
        device_ctx->current_cursor_line++;
        device_ctx->current_cursor_column = 0;
        if (device_ctx->current_cursor_line >= MAX_DISPLAY_LINES) {
            device_ctx->current_cursor_line = 0;
        }
        ssd1306_set_cursor_position(device_ctx, 
                                   device_ctx->current_cursor_line, 
                                   device_ctx->current_cursor_column);
    }
    
    /* Get font data - simple character mapping */
    if (character >= '0' && character <= '9') {
        font_data_ptr = font_table_5x8[character - '0' + 1]; /* +1 because space is at index 0 */
    } else if (character >= 'A' && character <= 'Z') {
        font_data_ptr = font_table_5x8[character - 'A' + 11];
    } else if (character >= 'a' && character <= 'z') {
        font_data_ptr = font_table_5x8[character - 'a' + 11]; /* Use uppercase font */
    } else {
        font_data_ptr = font_table_5x8[0]; /* Space for unknown characters */
    }
    
    /* Send font data to display */
    for (font_byte_index = 0; font_byte_index < 5; font_byte_index++) {
        ssd1306_send_i2c_data(device_ctx, font_data_ptr[font_byte_index]);
    }
    
    /* Add space between characters */
    ssd1306_send_i2c_data(device_ctx, 0x00);
    
    device_ctx->current_cursor_column++;
    
    return 0;
}

/**
 * @brief Write text string to display
 * @param device_ctx Pointer to device context structure
 * @param text_string Null-terminated text string to display
 * @return 0 on success, negative error code on failure
 */
int ssd1306_write_text_to_display(struct ssd1306_device_context *device_ctx, 
                                  const char *text_string)
{
    while (*text_string) {
        ssd1306_write_single_character(device_ctx, *text_string++);
    }
    return 0;
}

/**
 * @brief Set display brightness level
 * @param device_ctx Pointer to device context structure
 * @param brightness_level Brightness level (0-255)
 * @return 0 on success, negative error code on failure
 */
int ssd1306_set_display_brightness(struct ssd1306_device_context *device_ctx, 
                                   uint8_t brightness_level)
{
    ssd1306_send_i2c_command(device_ctx, SSD1306_CMD_SET_CONTRAST);
    ssd1306_send_i2c_command(device_ctx, brightness_level);
    device_ctx->display_brightness_level = brightness_level;
    return 0;
}

/* Character Device File Operations Implementation */

/**
 * @brief Character device open operation
 * @param inode_ptr Pointer to inode structure
 * @param file_ptr Pointer to file structure
 * @return 0 on success, negative error code on failure
 */
static int ssd1306_char_device_open(struct inode *inode_ptr, struct file *file_ptr)
{
    file_ptr->private_data = global_ssd1306_device;
    dev_info(&global_ssd1306_device->i2c_client_ptr->dev, 
             "SSD1306 character device opened\n");
    return 0;
}

/**
 * @brief Character device release operation
 * @param inode_ptr Pointer to inode structure
 * @param file_ptr Pointer to file structure
 * @return 0 on success, negative error code on failure
 */
static int ssd1306_char_device_release(struct inode *inode_ptr, struct file *file_ptr)
{
    dev_info(&global_ssd1306_device->i2c_client_ptr->dev, 
             "SSD1306 character device closed\n");
    return 0;
}

/**
 * @brief Character device write operation
 * @param file_ptr Pointer to file structure
 * @param user_buffer User space buffer containing data to write
 * @param write_count Number of bytes to write
 * @param file_position File position pointer
 * @return Number of bytes written or negative error code
 */
static ssize_t ssd1306_char_device_write(struct file *file_ptr, const char __user *user_buffer, 
                                         size_t write_count, loff_t *file_position)
{
    struct ssd1306_device_context *device_ctx = file_ptr->private_data;
    char message_buffer[MAX_MESSAGE_BUFFER_SIZE];
    size_t safe_write_count = min(write_count, (size_t)(MAX_MESSAGE_BUFFER_SIZE - 1));
    
    if (copy_from_user(message_buffer, user_buffer, safe_write_count)) {
        return -EFAULT;
    }
    
    message_buffer[safe_write_count] = '\0';
    
    dev_info(&device_ctx->i2c_client_ptr->dev, 
             "Writing text to display: %s\n", message_buffer);
    
    /* Clear screen and write new text */
    ssd1306_clear_display_screen(device_ctx);
    ssd1306_set_cursor_position(device_ctx, 0, 0);
    ssd1306_write_text_to_display(device_ctx, message_buffer);
    
    /* Save message to device buffer */
    strncpy(device_ctx->message_display_buffer, message_buffer, MAX_MESSAGE_BUFFER_SIZE - 1);
    device_ctx->message_display_buffer[MAX_MESSAGE_BUFFER_SIZE - 1] = '\0';
    
    return safe_write_count;
}

/**
 * @brief Character device read operation
 * @param file_ptr Pointer to file structure
 * @param user_buffer User space buffer to read data into
 * @param read_count Number of bytes to read
 * @param file_position File position pointer
 * @return Number of bytes read or negative error code
 */
static ssize_t ssd1306_char_device_read(struct file *file_ptr, char __user *user_buffer, 
                                        size_t read_count, loff_t *file_position)
{
    struct ssd1306_device_context *device_ctx = file_ptr->private_data;
    size_t buffer_length = strlen(device_ctx->message_display_buffer);
    
    if (*file_position >= buffer_length) {
        return 0; /* End of file */
    }
    
    if (read_count > buffer_length - *file_position) {
        read_count = buffer_length - *file_position;
    }
    
    if (copy_to_user(user_buffer, device_ctx->message_display_buffer + *file_position, read_count)) {
        return -EFAULT;
    }
    
    *file_position += read_count;
    return read_count;
}

/**
 * @brief Create character device file node
 * @param device_ctx Pointer to device context
 * @return 0 on success, negative error code on failure
 */
static int ssd1306_create_character_device(struct ssd1306_device_context *device_ctx)
{
    int result = 0;
    
    dev_info(&device_ctx->i2c_client_ptr->dev, 
             "Creating SSD1306 character device\n");
    
    /* Allocate character device region */
    result = alloc_chrdev_region(&device_ctx->char_device_number, 0, 1, DEVICE_NAME);
    if (result < 0) {
        dev_err(&device_ctx->i2c_client_ptr->dev, 
                "Failed to allocate character device region: %d\n", result);
        goto allocation_failed;
    }
    
    dev_info(&device_ctx->i2c_client_ptr->dev, 
             "Character device allocated: major=%d, minor=%d\n",
             MAJOR(device_ctx->char_device_number), 
             MINOR(device_ctx->char_device_number));
    
    /* Create device class */
    device_ctx->char_device_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);
    if (IS_ERR(device_ctx->char_device_class)) {
        result = PTR_ERR(device_ctx->char_device_class);
        dev_err(&device_ctx->i2c_client_ptr->dev, 
                "Failed to create device class: %d\n", result);
        goto class_creation_failed;
    }
    
    /* Create device node */
    device_ctx->char_device_node = device_create(device_ctx->char_device_class, NULL, 
                                                 device_ctx->char_device_number, 
                                                 NULL, 
                                                 DEVICE_NAME);
    if (IS_ERR(device_ctx->char_device_node)) {
        result = PTR_ERR(device_ctx->char_device_node);
        dev_err(&device_ctx->i2c_client_ptr->dev, 
                "Failed to create device node: %d\n", result);
        goto device_creation_failed;
    }
    
    /* Initialize and add character device */
    cdev_init(&device_ctx->char_device_cdev, &ssd1306_char_device_file_operations);
    device_ctx->char_device_cdev.owner = THIS_MODULE;
    
    result = cdev_add(&device_ctx->char_device_cdev, device_ctx->char_device_number, 1);
    if (result) {
        dev_err(&device_ctx->i2c_client_ptr->dev, 
                "Failed to add character device: %d\n", result);
        goto cdev_add_failed;
    }
    
    dev_info(&device_ctx->i2c_client_ptr->dev, 
             "Character device created successfully: /dev/%s\n", DEVICE_NAME);
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
 * @brief I2C probe callback function
 * @param client Pointer to I2C client structure
 * @param device_id Pointer to I2C device ID structure
 * @return 0 on success, negative error code on failure
 */
static int ssd1306_i2c_probe_callback(struct i2c_client *client, 
                                      const struct i2c_device_id *device_id)
{
    struct ssd1306_device_context *device_ctx;
    int result;
    
    dev_info(&client->dev, "SSD1306 I2C probe started\n");
    
    /* Allocate device context structure */
    device_ctx = devm_kzalloc(&client->dev, sizeof(*device_ctx), GFP_KERNEL);
    if (!device_ctx) {
        dev_err(&client->dev, "Failed to allocate device context memory\n");
        return -ENOMEM;
    }
    
    /* Initialize device context */
    device_ctx->i2c_client_ptr = client;
    i2c_set_clientdata(client, device_ctx);
    
    /* Initialize display hardware */
    result = ssd1306_initialize_display_hardware(device_ctx);
    if (result) {
        dev_err(&client->dev, "Failed to initialize display hardware: %d\n", result);
        return result;
    }
    
    /* Set cursor and display demo message */
    ssd1306_set_cursor_position(device_ctx, 0, 0);
    ssd1306_write_text_to_display(device_ctx, "HELLO SON TUNG\nSSD1306 Ready");
    
    /* Create character device */
    result = ssd1306_create_character_device(device_ctx);
    if (result) {
        dev_err(&client->dev, "Failed to create character device: %d\n", result);
        return result;
    }
    
    /* Set global device instance */
    global_ssd1306_device = device_ctx;
    
    dev_info(&client->dev, "SSD1306 probe completed successfully\n");
    return 0;
}

/**
 * @brief I2C remove callback function
 * @param client Pointer to I2C client structure
 * @return 0 on success, negative error code on failure
 */
static int ssd1306_i2c_remove_callback(struct i2c_client *client)
{
    struct ssd1306_device_context *device_ctx = i2c_get_clientdata(client);
    
    dev_info(&client->dev, "SSD1306 I2C remove started\n");
    
    /* Display goodbye message */
    ssd1306_write_text_to_display(device_ctx, "GOODBYE!\nShutdown...");
    msleep(1000);
    
    /* Clear display and turn off */
    ssd1306_clear_display_screen(device_ctx);
    ssd1306_send_i2c_command(device_ctx, SSD1306_CMD_DISPLAY_OFF);
    
    /* Clean up character device */
    cdev_del(&device_ctx->char_device_cdev);
    device_destroy(device_ctx->char_device_class, device_ctx->char_device_number);
    class_destroy(device_ctx->char_device_class);
    unregister_chrdev_region(device_ctx->char_device_number, 1);
    
    /* Clear global reference */
    global_ssd1306_device = NULL;
    
    dev_info(&client->dev, "SSD1306 I2C remove completed\n");
    return 0;
}

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