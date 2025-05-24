/**
 * @file led_driver.h
 * @brief LED character device driver for Raspberry Pi Zero W
 * @author TungNHS
 */

 #ifndef LED_DRIVER_H
 #define LED_DRIVER_H

 /* Driver constants */
 #define DRIVER_NAME            "led_driver"
 #define DEVICE_NAME            "led"
 #define CLASS_NAME             "led_class"

 /* BCM2835 GPIO registers (Pi Zero W) */
 #define BCM2835_GPIO_BASE      0x20200000
 #define GPIO_REGISTER_SIZE     0x100

 /* Register offsets */
 #define GPIO_FSEL0_OFFSET      0x00
 #define GPIO_FSEL1_OFFSET      0x04
 #define GPIO_SET0_OFFSET       0x1C
 #define GPIO_CLR0_OFFSET       0x28
 #define GPIO_LEV0_OFFSET       0x34

 /* GPIO configuration */
 #define GPIO_FUNCTION_INPUT    0
 #define GPIO_FUNCTION_OUTPUT   1
 #define DEFAULT_LED_GPIO       17

 /* LED commands */
 #define LED_CMD_OFF            '0'
 #define LED_CMD_ON             '1'

#endif /* LED_DRIVER_H */
