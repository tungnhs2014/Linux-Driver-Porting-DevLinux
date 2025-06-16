# Nokia 5110 LCD Write Application
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
LDFLAGS ?=

TARGET := lcd_write
SOURCES := lcd_write.c
HEADERS := lcd_write.h

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET) *.o

install: $(TARGET)
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/

.PHONY: all clean install