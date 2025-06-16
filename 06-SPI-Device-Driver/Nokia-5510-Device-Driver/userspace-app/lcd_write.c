/**
 * @file lcd_write.c
 * @brief Nokia 5110 LCD Display Controller
 * @author TungNHS
 * @version 1.0
 */

#include "lcd_write.h"

/**
 * @brief Print application usage information
 * @param program_name Name of the program executable
 */
void print_usage_information(const char *program_name)
{
    printf("Nokia 5110 LCD Display Controller\n");
    printf("Usage: %s <command> [text]\n\n", program_name);
    printf("Commands:\n");
    printf("  write <text>    - Write text to display\n");
    printf("  read            - Read current display content\n");
    printf("  clear           - Clear display\n");
    printf("  demo            - Display demo message 'NOKIA 5110 READY'\n");
}

/**
 * @brief Open Nokia 5110 device file
 * @return File descriptor on success, -1 on failure
 */
int open_nokia5110_device(void)
{
    int device_fd = open(NOKIA5110_DEVICE_PATH, O_RDWR);
    
    if (device_fd < 0) {
        printf("Error: Cannot open device %s\n", NOKIA5110_DEVICE_PATH);
        return -1;
    }
    
    return device_fd;
}

/**
 * @brief Write text message to LCD display
 * @param device_fd Device file descriptor
 * @param text_message Text message to display
 * @return 0 on success, -1 on failure
 */
int write_text_to_lcd(int device_fd, const char *text_message)
{
    ssize_t bytes_written;
    size_t message_length = strlen(text_message);
    
    printf("Writing to LCD: \"%s\"\n", text_message);
    
    bytes_written = write(device_fd, text_message, message_length);
    if (bytes_written < 0) {
        printf("Error: Failed to write to LCD display\n");
        return -1;
    }
    
    if ((size_t)bytes_written != message_length) {
        printf("Warning: Only %zd of %zu bytes written\n", bytes_written, message_length);
    }
    
    printf("Successfully wrote %zd bytes to LCD display\n", bytes_written);
    return 0;
}

/**
 * @brief Read current display content
 * @param device_fd Device file descriptor
 * @return 0 on success, -1 on failure
 */
int read_display_content(int device_fd)
{
    char read_buffer[MAX_INPUT_LENGTH];
    ssize_t bytes_read;
    
    printf("Reading current display content:\n");
    
    bytes_read = read(device_fd, read_buffer, sizeof(read_buffer) - 1);
    if (bytes_read < 0) {
        printf("Error: Failed to read from LCD display\n");
        return -1;
    }
    
    read_buffer[bytes_read] = '\0';
    printf("Display content: \"%s\"\n", read_buffer);
    printf("Read %zd bytes from display\n", bytes_read);
    
    return 0;
}

/**
 * @brief Clear LCD display
 * @param device_fd Device file descriptor
 * @return 0 on success, -1 on failure
 */
int clear_lcd_display(int device_fd)
{
    printf("Clearing LCD display...\n");
    
    /* Write empty string to clear display */
    if (write_text_to_lcd(device_fd, "") < 0) {
        return -1;
    }
    
    printf("Display cleared successfully\n");
    return 0;
}

/**
 * @brief Display demo message "NOKIA 5110 READY"
 * @param device_fd Device file descriptor
 * @return 0 on success, -1 on failure
 */
int display_demo_message(int device_fd)
{
    const char *demo_message = "NOKIA 5110\nREADY";
    
    printf("Displaying demo message...\n");
    
    if (write_text_to_lcd(device_fd, demo_message) < 0) {
        return -1;
    }
    
    printf("Demo message displayed successfully\n");
    return 0;
}

int main(int argc, char *argv[])
{
    int device_fd;
    int result = 0;
    
    /* Check minimum arguments */
    if (argc < 2) {
        print_usage_information(argv[0]);
        return 1;
    }
    
    /* Open device */
    device_fd = open_nokia5110_device();
    if (device_fd < 0) {
        return 1;
    }
    
    /* Process commands */
    if (strcmp(argv[1], "write") == 0) {
        if (argc < 3) {
            printf("Error: 'write' command requires text argument\n");
            print_usage_information(argv[0]);
            result = 1;
        } else {
            result = write_text_to_lcd(device_fd, argv[2]);
        }
    }
    else if (strcmp(argv[1], "read") == 0) {
        result = read_display_content(device_fd);
    }
    else if (strcmp(argv[1], "clear") == 0) {
        result = clear_lcd_display(device_fd);
    }
    else if (strcmp(argv[1], "demo") == 0) {
        result = display_demo_message(device_fd);
    }
    else {
        printf("Error: Unknown command '%s'\n", argv[1]);
        print_usage_information(argv[0]);
        result = 1;
    }
    
    /* Close device */
    close(device_fd);
    
    return (result == 0) ? 0 : 1;
}