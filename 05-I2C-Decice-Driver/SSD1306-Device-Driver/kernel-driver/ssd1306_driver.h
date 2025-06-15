/**
 * @file ssd1306_driver.h
 * @brief SSD1306 OLED Display Driver Interface
 * @author TungNHS
 * @version 1.0
 * 
 * Header file containing definition, constants and structures
 * for SSD1306 I2C OLED display driver
 */

#ifndef SSD1306_DRIVER_H
#define SSD1306_DRIVER_H

#include <linux/i2c.h>
#include <linux/cdev.h>

/* Display hardware constant */
#define DISPLAY_WIDTH_PIXELS        128    /* Display width in pixels */
#define DISPLAY_HEIGHT_PIXELS       64     /* Display height in pixels */
#define DISPLAY_TOTAL_PAGES         8      /* Total pages (64/8 = 8) */
#define FONT_CHAR_WIDTH             6      /* Character width (5 + 1 space) */
#define MAX_CHARS_PER_LINE          21     /* Max characters per line (128/6) */
#define MAX_DISPLAY_LINES           8      /* Maximum display lines */
#define MAX_MESSAGE_BUFFER_SIZE     256    /* Message buffer size */

/* Device naming constants */
#define DEVICE_NAME                 "ssd1306"
#define DEVICE_CLASS_NAME           "ssd1306_class"
#define I2C_DRIVER_NAME             "ssd1306-i2c"

/* I2C communication constants */
#define I2C_CMD_PREFIX              0x00    /* Command prefix */
#define I2C_DATA_PREFIX             0x40    /* Data prefix */

/* SSD1306 command definitions */
#define SSD1306_CMD_DISPLAY_OFF     0xAE   /* Display OFF */
#define SSD1306_CMD_DISPLAY_ON      0xAF   /* Display ON */
#define SSD1306_CMD_SET_CONTRAST    0x81   /* Set contrast */
#define SSD1306_CMD_SET_COLUMN_ADDR 0x21   /* Set column address */
#define SSD1306_CMD_SET_PAGE_ADDR   0x22   /* Set page address */

/**
 * @brief Main driver context structure
 * 
 * Main structure containing all driver state information
 */
struct ssd1306_device_context {
    /* I2C communication components */
    struct i2c_client *i2c_client_ptr;

    /* Character device components */
    struct device *char_device_node;
    struct class *char_device_class;
    struct cdev char_device_cdev;
    dev_t char_device_number;

    /* Display state management */
    uint8_t current_cursor_line;         
    uint8_t current_cursor_column;      
    char message_display_buffer[MAX_MESSAGE_BUFFER_SIZE];   // Display buffer
    
    /* Device configuration */  
    bool is_display_enabled;
    uint8_t display_brightness_level;
};

/* Function prototype */
int ssd1306_initialize_display_hardware(struct ssd1306_device_context *device_ctx);
int ssd1306_clear_display_screen(struct ssd1306_device_context *device_ctx);
int ssd1306_write_text_to_display(struct ssd1306_device_context *device_ctx, const char *text_string);
int ssd1306_set_display_brightness(struct ssd1306_device_context *device_ctx, uint8_t brightness_level);

#endif /* SSD1306_DRIVER_H */