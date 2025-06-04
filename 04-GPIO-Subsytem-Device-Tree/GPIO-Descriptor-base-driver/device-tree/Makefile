DTC = dtc
DTC_FLAGS = -@ -I dts -O dtb -W no-unit_address_vs_reg

SOURCE = gpio_led_overlay.dts
TARGET = gpio_led_overlay.dtbo

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(DTC) $(DTC_FLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f *.dtbo
