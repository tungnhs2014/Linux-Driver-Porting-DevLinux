/**
 * @file led_ctrl.h
 * @brief GPIO LED Controller
 */

#ifndef LED_CTRL_H
#define LED_CTRL_H

#define DEVICE_PATH "/dev/gpio_led"

/* Include driver IOCTL definitions */
#include "../kernel-driver/gpio_led_driver.h"

/* Function prototypes - Complete IOCTL coverage */
void print_usage(char *name);
int open_led_device(void);

/* Status and info commands */
int cmd_status(int fd);
int cmd_get_state(int fd);
int cmd_get_current(int fd);
int cmd_get_count(int fd);
int cmd_get_all_states(int fd);

/* LED control commands */
int cmd_led_on(int fd);
int cmd_led_off(int fd);
int cmd_toggle(int fd);
int cmd_blink(int fd, int count);

/* LED selection commands */
int cmd_select_led(int fd, int led_index);

/* Bulk control commands */
int cmd_all_on(int fd);
int cmd_all_off(int fd);

#endif /* LED_CTRL_H */