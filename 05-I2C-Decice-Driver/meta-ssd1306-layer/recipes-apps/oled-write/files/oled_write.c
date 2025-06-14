/**
 * @file oled_write.c
 * @brief SSD1306 OLED Display Controller
 * @author TungNHS
 * @version 1.0
 */

#include "oled_write.h"

void print_usage_information(const char *program_name) {
    printf("SSD1306 OLED Display Controller\n");
    printf("Usage: %s <command> [text]\n\n", program_name);
    printf("Commands:\n");
    printf("  write <text>    - Write text to display\n");
    printf("  read            - Read current display content\n");
    printf("  clear           - Clear display\n");
    printf("  demo            - Display demo message 'HELLO SON TUNG'\n");
}

int open_ssd1306_device(void) {
    int device_fd = open(SSD1306_DEVICE_PATH, O_RDWR);

    if (device_fd < 0) {
        printf("Error: Cannot open device %s\n", SSD1306_DEVICE_PATH);
        return -1;
    }

    return device_fd;
}

int write_text_to_oled(int device_fd, const char *text_message) {
    ssize_t bytes_written;
    size_t message_length = strlen(text_message);

    printf("Writting to OLED: \"%s\"\n", text_message);

    bytes_written = write(device_fd, text_message, message_length);
    if (bytes_written < 0) {
        printf("Error: Failed to write  OLED display\n");
        return -1;
    }

    if ((size_t)bytes_written != message_length) {
        printf("Warning: Only %zd of %zu bytes written\n", bytes_written, message_length);
    }

    printf("Successfully wrote %zd bytes to OLED display\n", bytes_written);

    return 0;
}

int read_display_content(int device_fd) {
    char read_buffer[MAX_INPUT_LENGTH];
    ssize_t bytes_read;

    printf("Reading current display content: \n");

    bytes_read = read(device_fd, read_buffer, sizeof(read_buffer) - 1);
    if (bytes_read < 0) {
        printf("Error: Failed to read froom OLED display");
        return -1;
    }

    read_buffer[bytes_read] = '\0';
    printf("Display content: \"%s\"\n", read_buffer);
    printf("Read %zd bytes from display\n", bytes_read);

    return 0;
}

int clear_oled_display(int device_fd) {
    printf("Clearing OLED display...\n");

    /* Write empty string to clear display */
    if (write_text_to_oled(device_fd, "") < 0) {
        return -1;
    }

    printf("Display cleared successfully\n");
    return 0;
}

int display_demo_message(int device_fd)
{
    const char *demo_message = "HELLO SON TUNG\nSSD1306 Demo";
    
    printf("Displaying demo message...\n");
    
    if (write_text_to_oled(device_fd, demo_message) < 0) {
        return -1;
    }
    
    printf("Demo message displayed successfully\n");
    return 0;
}

int main(int argc, char const *argv[])
{
    int device_fd;
    int result = 0;

    /* Check minimum argumemts */
    if (argc < 2) {
        print_usage_information(argv[0]);
        return 1;
    }

    /* Open device */
    device_fd = open_ssd1306_device();
    if (device_fd < 0) {
        return 1;
    }

    /* Process commands */
    if (strcmp(argv[1], "write") == 0) {
        if (argc < 3) {
            printf("Error: 'write' command requires text argument\n");
            print_usage_information(argv[0]);
            result = 1;
        }
        else {
            result = write_text_to_oled(device_fd, argv[2]);
        }
    }
    else if (strcmp(argv[1], "read") == 0) {
        result = read_display_content(device_fd);   
    }
    else if (strcmp(argv[1], "clear") == 0) {
        result = clear_oled_display(device_fd);   
    }
    else if (strcmp(argv[1], "demo") == 0) {
        result = display_demo_message(device_fd);   
    }
    else {
        printf("Error: Unknown command '%s\n", argv[1]);
        print_usage_information(argv[0]);
        result = 1;
    }

    /* Close device */
    close(device_fd);

    return (result == 0) ? 0 : 1;
}
