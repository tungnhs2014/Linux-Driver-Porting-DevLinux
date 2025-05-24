/**
 * @file gpio_led_driver.h
 * @brief GPIO LED character device driver using legacy GPIO interface
 */

#ifndef GPIO_LED_DRIVER_H
#define GPIO_LED_DRIVER_H

/* Driver constants */
#define DRIVER_NAME             "gpio_led_driver"
#define DEVICE_NAME             "gpio_led"
#define CLASS_NAME              "gpio_led_class"

/* GPIO configuration */
#define DEFAULT_LED_GPIO        18
#define GPIO_LOW                0
#define GPIO_HIGH               1

/* LED commands */
#define LED_CMD_OFF             '0'
#define LED_CMD_ON              '1'
#define LED_CMD_STATUS          's'
#define LED_CMD_TOGGLE          't'

/* IOCTL commands */
#define GPIO_LED_IOC_MAGIC      'g'
#define GPIO_LED_IOC_SET_PIN    _IOW(GPIO_LED_IOC_MAGIC, 1, int)
#define GPIO_LED_IOC_GET_PIN    _IOR(GPIO_LED_IOC_MAGIC, 2, int)
#define GPIO_LED_IOC_SET_STATE  _IOW(GPIO_LED_IOC_MAGIC, 3, int)
#define GPIO_LED_IOC_GET_STATE  _IOR(GPIO_LED_IOC_MAGIC, 4, int)

#endif /* GPIO_LED_DRIVER_H */