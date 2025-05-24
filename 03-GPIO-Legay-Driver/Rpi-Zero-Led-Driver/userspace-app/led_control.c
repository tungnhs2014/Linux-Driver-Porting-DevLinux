/**
 * @file led_control.c
 * @brief LED controller application
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <errno.h>

 #include "led_control.h"

 /**
  * @brief Set LED state
  * @param state true for ON, false for OFF
  * @return 0 on success, negative on error
  */
 int led_set_state(bool state) {
     int fd;
     char command = state ? '1' : '0';
    
     fd = open(DEVICE_PATH, O_WRONLY);
     if (fd < 0) {
         perror("Failed to open LED device");
         return ERROR_DEVICE;
     }
    
     if (write(fd, &command, 1) < 0) {
         perror("Failed to write to LED device");
         close(fd);
         return ERROR_OPERATION;
     }
    
     close(fd);
     return SUCCESS;
 }

 /**
  * @brief Get LED status
  * @return 0 on success, negative on error
  */
 int led_get_status(void) {
     int fd;
     char buffer[BUFFER_SIZE];
     ssize_t bytes_read;
     
     fd = open(DEVICE_PATH, O_RDONLY);
     if (fd < 0) {
         perror("Failed to open LED device");
         return ERROR_DEVICE;
     }
    
     bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
     if (bytes_read < 0) {
         perror("Failed to read from LED device");
         close(fd);
         return ERROR_OPERATION;
     }
    
     buffer[bytes_read] = '\0';
     printf("%s", buffer);
    
     close(fd);
     return SUCCESS;
 }

 /**
  * @brief Blink LED specified number of times
  * @param count Number of blinks
  * @param delay_ms Delay between state changes in milliseconds
  * @return 0 on success, negative on error
  */
 int led_blink(int count, int delay_ms) {
     int i, result;
    
     printf("Blinking LED %d times (delay: %dms)\n", count, delay_ms);
    
     for (i = 0; i < count; i++) {
         result = led_set_state(true);
         if (result != SUCCESS) return result;
        
         usleep(delay_ms * 1000);
        
         result = led_set_state(false);
         if (result != SUCCESS) return result;
        
         usleep(delay_ms * 1000);
         printf("Blink %d/%d completed\n", i + 1, count);
     }
    
     return SUCCESS;
 }

 /**
  * @brief Print usage
  */
 void print_usage(const char *program_name) {
     printf("LED Controller for Raspberry Pi Zero W\n\n");
     printf("Usage: %s <command> [options]\n\n", program_name);
     printf("Commands:\n");
     printf("  on              Turn LED ON\n");
     printf("  off             Turn LED OFF\n");
     printf("  status          Show LED status\n");
     printf("  blink           Blink LED 5 times\n");
     printf("  blink <count>   Blink LED count times\n");
     printf("  blink <count> <delay>  Blink with custom delay (ms)\n");
 }

 int main(int argc, char *argv[]) {
     int result = SUCCESS;
    
     if (argc < 2) {
         print_usage(argv[0]);
         return ERROR_ARGS;
     }
    
     if (strcmp(argv[1], "on") == 0) {
         result = led_set_state(true);
         if (result == SUCCESS) {
            printf("LED turned ON\n");
         } 
     }
     else if (strcmp(argv[1], "off") == 0) {
         result = led_set_state(false);
         if (result == SUCCESS) {
            printf("LED turned OFF\n");
         }
    }
    else if (strcmp(argv[1], "status") == 0) {
        result = led_get_status();
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
        
        result = led_blink(count, delay);
    }
    else {
        printf("Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
        return ERROR_ARGS;
    }
    
    return result;
}