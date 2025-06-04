/**
 * @file led_ctrl.h
 * @brief GPIO LED Controller
 */

#ifndef LED_CTRL_H
#define LED_CTRL_H

#define DEVICE_PATH "/dev/gpio_led"

/**
 * @brief IOCTL command definitions
 */
#define GPIO_LED_MAGIC 'G'

/* LED state control */
#define GPIO_LED_SET_STATE    _IOW(GPIO_LED_MAGIC, 1, int)
#define GPIO_LED_GET_STATE    _IOR(GPIO_LED_MAGIC, 2, int)
#define GPIO_LED_TOGGLE       _IOW(GPIO_LED_MAGIC, 3, int)

/* LED selection */
#define GPIO_LED_SELECT       _IOW(GPIO_LED_MAGIC, 4, int)
#define GPIO_LED_GET_CURRENT  _IOR(GPIO_LED_MAGIC, 5, int)

/* System info */
#define GPIO_LED_GET_COUNT    _IOR(GPIO_LED_MAGIC, 6, int)

/* Bulk operations */
#define GPIO_LED_SET_ALL      _IOW(GPIO_LED_MAGIC, 7, int)
#define GPIO_LED_GET_ALL      _IOR(GPIO_LED_MAGIC, 8, int)

#define GPIO_LED_MAX_CMD       8

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