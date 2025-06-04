/**
 * @file gpio_led_driver.h
 * @brief GPIO Descriptor LED Driver Interface
 * @author TungNHS
 * @version 1.0
 */

#ifndef GPIO_LED_DRIVER_H
#define GPIO_LED_DRIVER_H

#include <linux/ioctl.h>
#include <linux/types.h>

/* Driver constants */
#define DRIVER_NAME     "gpio_led_descriptor"
#define DEVICE_NAME     "gpio_led"
#define CLASS_NAME      "gpio_led_class"
#define MAX_LEDS        4

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

/**
 * @brief LED control structure
 */
struct led_control {
    int led_index;
    int state;
};

/**
 * @brief LED information structure
 */
struct led_info {
    int index;
    int state;
    char name[32];
    int gpio_num;
};

#endif /* GPIO_LED_DRIVER_H */