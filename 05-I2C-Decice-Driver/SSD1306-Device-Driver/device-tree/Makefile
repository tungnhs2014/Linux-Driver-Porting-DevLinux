# SSD1306 Device Tree

DTC := dtc
DTC_FLAGS := -@ -I dts -O dtb -W no-unit_address_vs_reg

SOURCE_FILE := ssd1306_overlay.dts
TARGET_FILE := ssd1306_overlay.dtbo

all: $(TARGET_FILE)

$(TARGET_FILE): $(SOURCE_FILE)
	$(DTC) $(DTC_FLAGS) -o $@ $<

clean:
	rm -f *.dtbo

.PHONY: all clean