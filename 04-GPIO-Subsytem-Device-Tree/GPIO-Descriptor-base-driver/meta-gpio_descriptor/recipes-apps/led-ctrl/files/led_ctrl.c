/**
 * @file led_ctrl.c
 * @brief GPIO LED Controller
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "led_ctrl.h"

/**
 * @brief Print usage information - Complete IOCTL commands
 */
void print_usage(char *name)
{
    printf("Usage: %s <command> [args]\n\n", name);
    printf("Status Commands:\n");
    printf("  status          - Show detailed LED status (read interface)\n");
    printf("  get-state       - Get current LED state (IOCTL)\n");
    printf("  get-current     - Get currently selected LED index\n");
    printf("  count           - Show total LED count\n");
    printf("  get-all         - Get all LED states as bitmask\n");
    printf("\nControl Commands:\n");
    printf("  on              - Turn current LED ON\n");
    printf("  off             - Turn current LED OFF\n");
    printf("  toggle          - Toggle current LED\n");
    printf("  blink [count]   - Blink LED (default: 5 times)\n");
    printf("\nSelection Commands:\n");
    printf("  select <index>  - Select LED by index (0 or 1)\n");
    printf("\nBulk Commands:\n");
    printf("  all-on          - Turn all LEDs ON\n");
    printf("  all-off         - Turn all LEDs OFF\n");
    printf("\nExamples:\n");
    printf("  %s status           # Detailed status via read()\n", name);
    printf("  %s get-state        # Current LED state via IOCTL\n", name);
    printf("  %s select 1         # Select LED 1\n", name);
    printf("  %s get-current      # Show selected LED\n", name);
    printf("  %s blink 3          # Blink 3 times\n", name);
    printf("  %s get-all          # Show all LED states\n", name);
}

/**
 * @brief Open LED device
 */
int open_led_device(void)
{
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        printf("Error: Cannot open %s\n", DEVICE_PATH);
        printf("Make sure driver is loaded: modprobe gpio_led_driver\n");
    }
    return fd;
}

/**
 * @brief Show LED status via read interface
 */
int cmd_status(int fd)
{
    char buf[256];
    int ret = read(fd, buf, sizeof(buf) - 1);
    if (ret > 0) {
        buf[ret] = '\0';
        printf("%s", buf);
        return 0;
    }
    printf("Error: Failed to read status\n");
    return -1;
}

/**
 * @brief Get current LED state via IOCTL
 */
int cmd_get_state(int fd)
{
    int state;
    int ret = ioctl(fd, GPIO_LED_GET_STATE, &state);
    if (ret == 0) {
        printf("Current LED state: %s\n", state ? "ON" : "OFF");
        return 0;
    }
    printf("Error: Failed to get LED state\n");
    return -1;
}

/**
 * @brief Get currently selected LED index
 */
int cmd_get_current(int fd)
{
    int current_led;
    int ret = ioctl(fd, GPIO_LED_GET_CURRENT, &current_led);
    if (ret == 0) {
        printf("Currently selected LED: %d\n", current_led);
        return 0;
    }
    printf("Error: Failed to get current LED\n");
    return -1;
}

/**
 * @brief Get all LED states as bitmask
 */
int cmd_get_all_states(int fd)
{
    int all_states;
    int ret = ioctl(fd, GPIO_LED_GET_ALL, &all_states);
    if (ret == 0) {
        printf("All LED states (bitmask): 0x%02X\n", all_states);
        
        /* Decode bitmask for better readability */
        printf("Individual states:\n");
        for (int i = 0; i < 4; i++) {  /* MAX_LEDS = 4 */
            if (all_states & (1 << i)) {
                printf("  LED %d: ON\n", i);
            } else {
                printf("  LED %d: OFF\n", i);
            }
        }
        return 0;
    }
    printf("Error: Failed to get all LED states\n");
    return -1;
}

/**
 * @brief Turn LED ON
 */
int cmd_led_on(int fd)
{
    int state = 1;
    int ret = ioctl(fd, GPIO_LED_SET_STATE, &state);
    if (ret == 0) {
        printf("LED ON\n");
        return 0;
    }
    printf("Error: Failed to turn LED ON\n");
    return -1;
}

/**
 * @brief Turn LED OFF
 */
int cmd_led_off(int fd)
{
    int state = 0;
    int ret = ioctl(fd, GPIO_LED_SET_STATE, &state);
    if (ret == 0) {
        printf("LED OFF\n");
        return 0;
    }
    printf("Error: Failed to turn LED OFF\n");
    return -1;
}

/**
 * @brief Toggle LED
 */
int cmd_toggle(int fd)
{
    int ret = ioctl(fd, GPIO_LED_TOGGLE, NULL);
    if (ret == 0) {
        printf("LED toggled\n");
        return 0;
    }
    printf("Error: Failed to toggle LED\n");
    return -1;
}

/**
 * @brief Blink LED
 */
int cmd_blink(int fd, int count)
{
    printf("Blinking %d times...\n", count);
    
    for (int i = 0; i < count; i++) {
        int state = 1;
        ioctl(fd, GPIO_LED_SET_STATE, &state);
        usleep(500000);  // 500ms ON
        
        state = 0;
        ioctl(fd, GPIO_LED_SET_STATE, &state);
        usleep(500000);  // 500ms OFF
    }
    
    printf("Blink completed\n");
    return 0;
}

/**
 * @brief Select LED
 */
int cmd_select_led(int fd, int led_index)
{
    int ret = ioctl(fd, GPIO_LED_SELECT, &led_index);
    if (ret == 0) {
        printf("Selected LED %d\n", led_index);
        return 0;
    }
    printf("Error: Failed to select LED %d\n", led_index);
    return -1;
}

/**
 * @brief Get LED count
 */
int cmd_get_count(int fd)
{
    int count;
    int ret = ioctl(fd, GPIO_LED_GET_COUNT, &count);
    if (ret == 0) {
        printf("LED count: %d\n", count);
        return 0;
    }
    printf("Error: Failed to get LED count\n");
    return -1;
}

/**
 * @brief Turn all LEDs ON
 */
int cmd_all_on(int fd)
{
    int state = 1;
    int ret = ioctl(fd, GPIO_LED_SET_ALL, &state);
    if (ret == 0) {
        printf("All LEDs ON\n");
        return 0;
    }
    printf("Error: Failed to turn all LEDs ON\n");
    return -1;
}

/**
 * @brief Turn all LEDs OFF
 */
int cmd_all_off(int fd)
{
    int state = 0;
    int ret = ioctl(fd, GPIO_LED_SET_ALL, &state);
    if (ret == 0) {
        printf("All LEDs OFF\n");
        return 0;
    }
    printf("Error: Failed to turn all LEDs OFF\n");
    return -1;
}

/**
 * @brief Main function - Complete IOCTL command dispatcher
 */
int main(int argc, char *argv[])
{
    int fd, ret = 0;
    
    /* Check arguments */
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    /* Open device */
    fd = open_led_device();
    if (fd < 0) {
        return 1;
    }
    
    /* Execute commands - Complete IOCTL coverage */
    
    /* Status and Info Commands */
    if (strcmp(argv[1], "status") == 0) {
        ret = cmd_status(fd);
    }
    else if (strcmp(argv[1], "get-state") == 0) {
        ret = cmd_get_state(fd);
    }
    else if (strcmp(argv[1], "get-current") == 0) {
        ret = cmd_get_current(fd);
    }
    else if (strcmp(argv[1], "count") == 0) {
        ret = cmd_get_count(fd);
    }
    else if (strcmp(argv[1], "get-all") == 0) {
        ret = cmd_get_all_states(fd);
    }
    
    /* Control Commands */
    else if (strcmp(argv[1], "on") == 0) {
        ret = cmd_led_on(fd);
    }
    else if (strcmp(argv[1], "off") == 0) {
        ret = cmd_led_off(fd);
    }
    else if (strcmp(argv[1], "toggle") == 0) {
        ret = cmd_toggle(fd);
    }
    else if (strcmp(argv[1], "blink") == 0) {
        int count = (argc > 2) ? atoi(argv[2]) : 5;
        ret = cmd_blink(fd, count);
    }
    
    /* Selection Commands */
    else if (strcmp(argv[1], "select") == 0) {
        if (argc < 3) {
            printf("Error: select needs LED index (0 or 1)\n");
            ret = -1;
        } else {
            int led_idx = atoi(argv[2]);
            ret = cmd_select_led(fd, led_idx);
        }
    }
    
    /* Bulk Commands */
    else if (strcmp(argv[1], "all-on") == 0) {
        ret = cmd_all_on(fd);
    }
    else if (strcmp(argv[1], "all-off") == 0) {
        ret = cmd_all_off(fd);
    }
    
    /* Invalid command */
    else {
        printf("Error: Unknown command '%s'\n", argv[1]);
        print_usage(argv[0]);
        ret = -1;
    }
    
    /* Clean up */
    close(fd);
    return (ret == 0) ? 0 : 1;
}