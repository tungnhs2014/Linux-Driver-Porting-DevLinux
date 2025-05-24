/**
 * @file led_control.h
 * @brief LED controller application header
 * @author TungNHS
 */

 #ifndef LED_CONTROL_H
 #define LED_CONTROL_H

 #include <stdbool.h>

 #define DEVICE_PATH     "/dev/led"
 #define BUFFER_SIZE     256

 /* Return codes */
 #define SUCCESS         0
 #define ERROR_ARGS      1
 #define ERROR_DEVICE    2
 #define ERROR_OPERATION 3

 /* Function prototypes */
 int led_set_state(bool state);
 int led_get_status(void);
 int led_blink(int count, int delay_ms);
 void print_usage(const char *program_name);

 #endif /* LED_CONTROL_H */