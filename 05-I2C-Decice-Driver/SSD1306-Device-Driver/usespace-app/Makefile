# SSD1306 OLED Write Application
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
LDFLAGS ?=

TARGET = oled_write
SOURCES = oled_write.c
HEADERS = oled_write.h

all: ${TARGET}

${TARGET}: ${SOURCES} ${HEADERS}
	${CC} ${CFLAGS} ${LDFLAGS} -o ${TARGET} ${SOURCES}

install: $(TARGET)
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/

clean:
	rm -rf ${TARGET} *.o

.PHONY: all clean install