/**
 * @file lcd_write.h
 * @brief Nokia 5110 LCD Display Controller Header
 * @author TungNHS
 * @version 1.0
 */

#ifndef LCD_WRITE_H
#define LCD_WRITE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Device file path */
#define NOKIA5110_DEVICE_PATH "/dev/nokia5110"

/* Application constants */
#define MAX_INPUT_LENGTH    256
#define MAX_DISPLAY_LINES   6
#define MAX_LINE_LENGTH     14

/* Function prototypes */
void print_usage_information(const char *program_name);
int open_nokia5110_device(void);
int write_text_to_lcd(int device_fd, const char *text_message);
int read_display_content(int device_fd);
int clear_lcd_display(int device_fd);
int display_demo_message(int device_fd);

#endif /* LCD_WRITE_H */