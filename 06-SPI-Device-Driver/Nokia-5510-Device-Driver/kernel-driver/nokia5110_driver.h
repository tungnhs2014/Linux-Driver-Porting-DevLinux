/**
 * @file nokia5110_driver.h
 * @brief Nokia 5110 LCD Display Driver Interface
 * @author TungNHS
 * @version 1.0
 */

#ifndef NOKIA5110_DRIVER_H
#define NOKIA5110_DRIVER_H

#include <linux/spi/spi.h>
#include <linux/cdev.h>
#include <linux/gpio.h>

/* Display hardware constants */
#define DISPLAY_WIDTH_PIXELS        84      // Display with in pixels
#define DISPLAY_HEIGHT_PIXELS       48      // Display height in pixels
#define DISPLAY_TOTAL_BANKS         6       // Total banks (48/8 = 6)
#define FONT_CHAR_WIDTH             6       // Character width (5 + 1 space)
#define MAX_CHARS_PER_LINE          14      // Max characters per line (84/6)
#define MAX_DISPLAY_LINES           6       // Maximum display lines
#define MAX_MESSAGE_BUFFER_SIZE     256     // Message buffer size

/* Device naming constants */
#define DEVICE_NAME                 "nokia5110"
#define DEVICE_CLASS_NAME           "nokia5110_class"
#define SPI_DRIVER_NAME             "nokia5110-spi"

/* LCD command constants */
#define LCD_CMD_FUNCTION_SET        0x20    // Function set command
#define LCD_CMD_EXTENDED_INSTR      0x21    // Extended instruction set
#define LCD_CMD_DISPLAY_CONTROL     0x0C    // Display control
#define LCD_CMD_SET_Y_ADDRESS       0x40    // Set Y address (0-5)
#define LCD_CMD_SET_X_ADDRESS       0x80    // Set X address (0-83)
#define LCD_CMD_CONTRAST            0xB1    // Set contrast
#define LCD_CMD_TEMP_COEFF          0x04    // Temperature coefficient
#define LCD_CMD_BIAS_SYSTEM         0x14    // Bias system

/* GPIO control constant */
#define GPIO_HIGH                   1       // GPIO high level
#define GPIO_LOW                    0       // GPIO low level

/**
 * @brief Main driver context structure
 * 
 * Main structure containing all driver state information
 * Using clear nameing convention for easy understanding
 */
struct nokia5110_device_context {
    /*SPI communication components */
    struct spi_device *spi_device_ptr;

    /* Character device components */
    struct device *char_device_node;
    struct class *char_device_class;
    struct cdev char_device_cdev;
    dev_t char_device_number;

    /* GPIO control pins */
    int reset_gpio_pin;             // Reset control pin number
    int dc_gpio_pin;                // Data/Command GPIO pin number

    /* Display state management */
    uint8_t current_cursor_x;       // Current cursor X position
    uint8_t current_cursor_y;       // Current cursor Y position
    char message_display_buffer[MAX_MESSAGE_BUFFER_SIZE]; // Display buffer

    /* Device configuration */
    bool is_display_enabled;         // Display enable/disable state
    uint8_t display_contrast_level;  // Display contrast level
};

/* Function prototypes with clear name */
int nokia5110_initialize_display_hardware(struct nokia5110_device_context *device_ctx);
int nokia5110_clear_display_screen(struct nokia5110_device_context *device_ctx);
int nokia5110_write_text_to_display(struct nokia5110_device_context *device_ctx, const char *text_string);
int nokia5110_set_display_contrast(struct nokia5110_device_context *device_ctx, uint8_t contrast_level);

#endif /* NOKIA5110_DRIVER_H */