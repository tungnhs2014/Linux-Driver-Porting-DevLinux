/**
 * @file oled_write.h
 * @brief SSD1306 OLED Display Controller Header
 * @author TungNHS
 * @version 1.0
 */

#ifndef OLED_WIRTE_H
#define OLED_WRITE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Device file path */
#define SSD1306_DEVICE_PATH "/dev/ssd1306"

/* Application constants */
#define MAX_INPUT_LENGTH       256
#define MAX_DISPLAY_LINES      8
#define MAX_LINE_LENGTH        21

/* Function prototypes */

/**
 * @brief Print application usage information
 * @param program_name Name of the program executable
 */
void print_usage_information(const char *program_name);

/**
 * @brief Open SSD1306 device file
 * @return File descriptor on success, -1 on failure
 */
int open_ssd1306_device(void);

/**
 * @brief Write text message to OLED display
 * @param device_fd Device file descriptor
 * @param text_message Text message to display
 * @return 0 on success, -1 on failure
 */
int write_text_to_oled(int device_fd, const char *text_message);

/**
 * @brief Read current display content
 * @param device_fd Device file descriptor
 * @return 0 on success, -1 on failure
 */
int read_display_content(int device_fd);

/**
 * @brief Clear OLED display
 * @param device_fd Device file descriptor
 * @return 0 on success, -1 on failure
 */
int clear_oled_display(int device_fd);

/**
 * @brief Display demo message "HELLO SON TUNG"
 * @param device_fd Device file descriptor
 * @return 0 on success, -1 on failure
 */
int display_demo_message(int device_fd);

#endif /* OLED_WIRTE_H */