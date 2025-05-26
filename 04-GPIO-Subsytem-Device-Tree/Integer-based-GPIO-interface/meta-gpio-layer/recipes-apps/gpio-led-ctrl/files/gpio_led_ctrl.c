/**
 * @file gpio_led_ctrl.c
 * @brief GPIO LED controller application using legacy GPIO interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "gpio_led_ctrl.h"

/**
 * @brief Set LED state using write interface
 */
int gpio_led_set_state(bool state)
{
    int fd;
    char command = state ? '1' : '0';
    
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open GPIO LED device");
        return ERROR_DEVICE;
    }
    
    if (write(fd, &command, 1) < 0) {
        perror("Failed to write to GPIO LED device");
        close(fd);
        return ERROR_OPERATION;
    }
    
    close(fd);
    return SUCCESS;
}

/**
 * @brief Get LED status using read interface
 */
int gpio_led_get_status(void)
{
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open GPIO LED device");
        return ERROR_DEVICE;
    }
    
    bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("Failed to read from GPIO LED device");
        close(fd);
        return ERROR_OPERATION;
    }
    
    buffer[bytes_read] = '\0';
    printf("%s", buffer);
    
    close(fd);
    return SUCCESS;
}

/**
 * @brief Toggle LED state
 */
int gpio_led_toggle(void)
{
    int fd;
    char command = 't';
    
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open GPIO LED device");
        return ERROR_DEVICE;
    }
    
    if (write(fd, &command, 1) < 0) {
        perror("Failed to toggle GPIO LED");
        close(fd);
        return ERROR_OPERATION;
    }
    
    close(fd);
    printf("LED toggled\n");
    return SUCCESS;
}

/**
 * @brief Set GPIO pin using ioctl
 */
int gpio_led_set_pin(int pin)
{
    int fd;
    
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open GPIO LED device");
        return ERROR_DEVICE;
    }
    
    if (ioctl(fd, GPIO_LED_IOC_SET_PIN, &pin) < 0) {
        perror("Failed to set GPIO pin");
        close(fd);
        return ERROR_OPERATION;
    }
    
    close(fd);
    printf("GPIO pin set to %d\n", pin);
    return SUCCESS;
}

/**
 * @brief Get current GPIO pin using ioctl
 */
int gpio_led_get_pin(void)
{
    int fd, pin;
    
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open GPIO LED device");
        return ERROR_DEVICE;
    }
    
    if (ioctl(fd, GPIO_LED_IOC_GET_PIN, &pin) < 0) {
        perror("Failed to get GPIO pin");
        close(fd);
        return ERROR_OPERATION;
    }
    
    close(fd);
    printf("Current GPIO pin: %d\n", pin);
    return SUCCESS;
}

/**
 * @brief Set LED state using ioctl
 */
int gpio_led_ioctl_set_state(bool state)
{
    int fd;
    int value = state ? 1 : 0;
    
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open GPIO LED device");
        return ERROR_DEVICE;
    }
    
    if (ioctl(fd, GPIO_LED_IOC_SET_STATE, &value) < 0) {
        perror("Failed to set LED state via ioctl");
        close(fd);
        return ERROR_OPERATION;
    }
    
    close(fd);
    printf("LED turned %s (via ioctl)\n", state ? "ON" : "OFF");
    return SUCCESS;
}

/**
 * @brief Get LED state using ioctl
 */
int gpio_led_ioctl_get_state(void)
{
    int fd, state;
    
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open GPIO LED device");
        return ERROR_DEVICE;
    }
    
    if (ioctl(fd, GPIO_LED_IOC_GET_STATE, &state) < 0) {
        perror("Failed to get LED state via ioctl");
        close(fd);
        return ERROR_OPERATION;
    }
    
    close(fd);
    printf("LED state: %s (via ioctl)\n", state ? "ON" : "OFF");
    return SUCCESS;
}

/**
 * @brief Blink LED
 */
int gpio_led_blink(int count, int delay_ms)
{
    int i, result;
    
    printf("Blinking LED %d times (delay: %dms)\n", count, delay_ms);
    
    for (i = 0; i < count; i++) {
        result = gpio_led_set_state(true);
        if (result != SUCCESS) return result;
        
        usleep(delay_ms * 1000);
        
        result = gpio_led_set_state(false);
        if (result != SUCCESS) return result;
        
        usleep(delay_ms * 1000);
        printf("Blink %d/%d completed\n", i + 1, count);
    }
    
    return SUCCESS;
}

/**
 * @brief Print usage
 */
void print_usage(const char *program_name)
{
    printf("GPIO LED Controller for Raspberry Pi Zero W (Legacy GPIO Interface)\n\n");
    printf("Usage: %s <command> [options]\n\n", program_name);
    printf("Basic Commands:\n");
    printf("  on              Turn LED ON\n");
    printf("  off             Turn LED OFF\n");
    printf("  status          Show LED status\n");
    printf("  toggle          Toggle LED state\n");
    printf("  blink           Blink LED 5 times\n");
    printf("  blink <count>   Blink LED count times\n");
    printf("  blink <count> <delay>  Blink with custom delay (ms)\n\n");
    printf("Advanced Commands (IOCTL):\n");
    printf("  setpin <pin>    Set GPIO pin number\n");
    printf("  getpin          Get current GPIO pin\n");
    printf("  ion             Turn LED ON (via ioctl)\n");
    printf("  ioff            Turn LED OFF (via ioctl)\n");
    printf("  istate          Get LED state (via ioctl)\n\n");
    printf("Examples:\n");
    printf("  %s on\n", program_name);
    printf("  %s blink 10 200\n", program_name);
    printf("  %s setpin 27\n", program_name);
    printf("  %s ion\n", program_name);
}

/**
 * @brief Main function
 */
int main(int argc, char *argv[])
{
    int result = SUCCESS;
    
    if (argc < 2) {
        print_usage(argv[0]);
        return ERROR_ARGS;
    }
    
    if (strcmp(argv[1], "on") == 0) {
        result = gpio_led_set_state(true);
    }
    else if (strcmp(argv[1], "off") == 0) {
        result = gpio_led_set_state(false);
    }
    else if (strcmp(argv[1], "status") == 0) {
        result = gpio_led_get_status();
    }
    else if (strcmp(argv[1], "toggle") == 0) {
        result = gpio_led_toggle();
    }
    else if (strcmp(argv[1], "ion") == 0) {
        result = gpio_led_ioctl_set_state(true);
    }
    else if (strcmp(argv[1], "ioff") == 0) {
        result = gpio_led_ioctl_set_state(false);
    }
    else if (strcmp(argv[1], "istate") == 0) {
        result = gpio_led_ioctl_get_state();
    }
    else if (strcmp(argv[1], "setpin") == 0) {
        if (argc < 3) {
            printf("Error: GPIO pin number required\n");
            return ERROR_ARGS;
        }
        int pin = atoi(argv[2]);
        if (pin < 0 || pin > 27) {
            printf("Error: GPIO pin must be 0-27\n");
            return ERROR_ARGS;
        }
        result = gpio_led_set_pin(pin);
    }
    else if (strcmp(argv[1], "getpin") == 0) {
        result = gpio_led_get_pin();
    }
    else if (strcmp(argv[1], "blink") == 0) {
        int count = 5, delay = 500;
        
        if (argc >= 3) {
            count = atoi(argv[2]);
            if (count <= 0) {
                printf("Error: Invalid count\n");
                return ERROR_ARGS;
            }
        }
        
        if (argc >= 4) {
            delay = atoi(argv[3]);
            if (delay <= 0) {
                printf("Error: Invalid delay\n");
                return ERROR_ARGS;
            }
        }
        
        result = gpio_led_blink(count, delay);
    }
    else {
        printf("Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
        return ERROR_ARGS;
    }
    
    return result;
}