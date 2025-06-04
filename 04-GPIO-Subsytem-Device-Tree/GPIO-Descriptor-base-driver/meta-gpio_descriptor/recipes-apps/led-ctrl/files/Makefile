# LED Controller Application Makefile
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -std=c99
TARGET = led_ctrl
SOURCES = led_ctrl.c
HEADERS = led_ctrl.h

# Use variables provided by Yocto
all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET) *.o

# Install target for Yocto
install: $(TARGET)
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/

.PHONY: all clean install