# LED Control Application Makefile
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -std=c99
LDFLAGS ?= 

TARGET = led_control
SOURCES = led_control.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJECTS)

clean:
	rm -f $(TARGET) $(OBJECTS)

install: $(TARGET)
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/

.PHONY: all clean install
