/**
 * @file gpio_led_ctrl.h
 * @brief GPIO LED controller application header
 */

#ifndef GPIO_LED_CTRL_H
#define GPIO_LED_CTRL_H

#include <stdbool.h>
#include <sys/ioctl.h>

#define DEVICE_PATH     "/dev/gpio_led"
#define BUFFER_SIZE     256

/* Return codes */
#define SUCCESS         0
#define ERROR_ARGS      1
#define ERROR_DEVICE    2
#define ERROR_OPERATION 3

/* IOCTL commands (match kernel driver) */
#define GPIO_LED_IOC_MAGIC      'g'
#define GPIO_LED_IOC_SET_PIN    _IOW(GPIO_LED_IOC_MAGIC, 1, int)
#define GPIO_LED_IOC_GET_PIN    _IOR(GPIO_LED_IOC_MAGIC, 2, int)
#define GPIO_LED_IOC_SET_STATE  _IOW(GPIO_LED_IOC_MAGIC, 3, int)
#define GPIO_LED_IOC_GET_STATE  _IOR(GPIO_LED_IOC_MAGIC, 4, int)

/* Function prototypes */
int gpio_led_set_state(bool state);
int gpio_led_get_status(void);
int gpio_led_blink(int count, int delay_ms);
int gpio_led_toggle(void);
int gpio_led_set_pin(int pin);
int gpio_led_get_pin(void);
int gpio_led_ioctl_set_state(bool state);
int gpio_led_ioctl_get_state(void);
void print_usage(const char *program_name);

#endif /* GPIO_LED_CTRL_H */