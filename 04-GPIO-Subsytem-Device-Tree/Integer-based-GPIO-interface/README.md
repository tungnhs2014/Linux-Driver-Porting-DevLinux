# GPIO Legacy Interface Project for Raspberry Pi Zero W

## Overview

This project creates a complete LED control system for Raspberry Pi Zero W using the Linux GPIO subsystem (legacy integer-based GPIO interface). The system includes a kernel driver that creates a character device and a userspace application to control the LED.

**Features:**
- Character device `/dev/gpio_led` for LED control
- Uses Linux GPIO subsystem for safe hardware access
- Multiple control interfaces: read/write and IOCTL
- Runtime GPIO pin configuration
- Commands: on/off/status/toggle/blink
- Auto-load kernel module at boot
- Full Yocto Project integration

## System Architecture

```
┌─────────────────┐    ┌──────────────┐    ┌─────────────────┐    ┌──────────────┐
│ User Commands   │◄──►│ /dev/gpio_led│◄──►│ Kernel Driver   │◄──►│ GPIO18 (LED) │
│gpio_led_ctrl on │    │  (chardev)   │    │ (gpio_led_drv)  │    │   Hardware   │
│                 │    │              │    │ + Linux GPIO    │    │              │
└─────────────────┘    └──────────────┘    └─────────────────┘    └──────────────┘
```

## Hardware Setup

```
Pi Zero W Pin 12 (GPIO18) ──[330Ω Resistor]──[LED Anode]
Pi Zero W Pin 9  (GND)     ────────────────────[LED Cathode]
```

**Note**: Default GPIO18, but can be changed at runtime using IOCTL commands.

## Project Structure

```
rpi-zero-gpio-legacy/
├── README.md
├── kernel-module/
│   ├── gpio_led_driver.c     # Kernel driver source
│   ├── gpio_led_driver.h     # Driver header  
│   ├── Makefile              # Driver Makefile
│   └── Kconfig               # Kernel config
├── userspace-app/
│   ├── gpio_led_ctrl.c       # App source
│   ├── gpio_led_ctrl.h       # App header
│   └── Makefile              # App Makefile
└── meta-gpio-layer/          # Yocto meta layer
    ├── conf/
    │   └── layer.conf
    ├── recipes-kernel/
    │   └── linux/
    │       ├── linux-raspberrypi_%.bbappend
    │       └── linux-raspberrypi/
    │           └── 0001-Add-GPIO-legacy-driver.patch
    └── recipes-apps/
        └── gpio-led-ctrl/
            ├── gpio-led-ctrl_1.0.bb
            └── files/
                ├── gpio_led_ctrl.c
                ├── gpio_led_ctrl.h
                └── Makefile
```

# Build Process with Yocto Project

## Part 1: Setup Yocto Environment

### 1.1. Prepare Environment

```bash
# Install required packages
sudo apt-get update
sudo apt-get install -y gawk wget git diffstat unzip texinfo gcc build-essential \
     chrpath socat cpio python3 python3-pip python3-pexpect xz-utils debianutils \
     iputils-ping python3-git python3-jinja2 libegl1-mesa libsdl1.2-dev pylint3 \
     xterm python3-subunit mesa-common-dev zstd liblz4-tool libgl-dev

# Setup Git
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"
```

### 1.2. Download and Setup Yocto

```bash
# Create workspace
mkdir ~/yocto-gpio-legacy-build
cd ~/yocto-gpio-legacy-build

# Clone Yocto and layers
git clone git://git.yoctoproject.org/poky -b dunfell
git clone git://git.yoctoproject.org/meta-raspberrypi -b dunfell

# Copy meta-layer to workspace
cp -r ~/rpi-zero-gpio-legacy/meta-gpio-layer .

# Setup build environment
cd poky
source oe-init-build-env
```

### 1.3. Configure Layers

```bash
# Add layers
bitbake-layers add-layer ../../meta-raspberrypi
bitbake-layers add-layer ../../meta-gpio-layer

# Check layers
bitbake-layers show-layers
```

## Part 2: Create Kernel Patch

### 2.1. Extract Kernel Source with DevTool

```bash
# Extract kernel source for changes
devtool modify virtual/kernel

# Go to kernel source
cd workspace/sources/linux-raspberrypi

# Check git status
git status
git log --oneline -3
```

### 2.2. Add GPIO Driver to Kernel Source Tree

```bash
# Create folder for driver
mkdir -p drivers/gpio_led_driver

# Copy source files from project
cp ~/rpi-zero-gpio-legacy/kernel-module/* drivers/gpio_led_driver/

# Update kernel build system
echo 'obj-$(CONFIG_GPIO_LED_DRIVER) += gpio_led_driver/' >> drivers/Makefile
echo 'source "drivers/gpio_led_driver/Kconfig"' >> drivers/Kconfig

# Add config to defconfig (loadable module)
echo 'CONFIG_GPIO_LED_DRIVER=m' >> arch/arm/configs/bcmrpi_defconfig
```

### 2.3. Commit and Create Patch

```bash
# Add files to git
git add drivers/gpio_led_driver/
git add drivers/Makefile
git add drivers/Kconfig
git add arch/arm/configs/bcmrpi_defconfig

# Save changes
git commit -m "Add GPIO LED character device driver using Linux GPIO subsystem

- Uses gpio_request, gpio_direction_output, gpio_set_value APIs
- Character device interface /dev/gpio_led
- Support for LED on/off/toggle control
- IOCTL interface for advanced control
- Runtime GPIO pin configuration
- Auto-load module configuration"

# Create patch file
git format-patch -1

# Copy patch to meta-layer
cp 0001-*.patch ~/yocto-gpio-legacy-build/meta-gpio-layer/recipes-kernel/linux/files/
```

### 2.4. Reset DevTool Workspace

```bash
# Go back to build folder
cd ~/yocto-gpio-legacy-build/poky/build

# Reset devtool
devtool reset virtual/kernel

# Check workspace is reset
devtool status
```

## Part 3: Configure Build

### 3.1. Configure local.conf

**Edit `conf/local.conf` with these settings:**

```bash
# Machine Setup
MACHINE = "raspberrypi0-wifi"
ENABLE_UART = "1"

# Network Support
DISTRO_FEATURES_append = " wifi bluetooth"

# GPIO Project packages (IMPORTANT: Only 1 IMAGE_INSTALL_append line)
IMAGE_INSTALL_append = " linux-firmware-bcm43430 wpa-supplicant dhcpcd kernel-modules gpio-led-ctrl"

# Development features
EXTRA_IMAGE_FEATURES += "ssh-server-dropbear debug-tweaks"

# Image format
IMAGE_FSTYPES = "tar.xz ext4 rpi-sdimg"

# Build optimization
BB_NUMBER_THREADS = "4"
PARALLEL_MAKE = "-j 4"
INHERIT += "rm_work"
RM_WORK_EXCLUDE += "linux-raspberrypi kernel-module-*"
```

**⚠️ IMPORTANT NOTE:**
- **Use only 1 line with `IMAGE_INSTALL_append`** 
- Multiple `_append` lines will overwrite each other (only last line works)
- If you want separate lines, use `IMAGE_INSTALL +=` instead of `_append`

### 3.2. Copy Userspace Files to Meta-Layer

```bash
# Copy userspace files to meta-layer in build workspace
cp ~/rpi-zero-gpio-legacy/userspace-app/* \
   ~/yocto-gpio-legacy-build/meta-gpio-layer/recipes-apps/gpio-led-ctrl/files/
```

## Part 4: Build Process

### 4.1. Build Kernel with Driver

```bash
cd ~/yocto-gpio-legacy-build/poky/build

# Build kernel
bitbake virtual/kernel

# Check kernel module is built
find tmp/work/ -name "gpio_led_driver.ko"
```

### 4.2. Build Userspace Application

```bash
# Build GPIO LED control app
bitbake gpio-led-ctrl

# If compile error happens, force rebuild:
bitbake gpio-led-ctrl -c compile -f

# Check executable is created
find tmp/work/ -name "gpio_led_ctrl" -executable
```

**Troubleshooting:** If executable not found:
- Build process might be stuck due to Yocto cache
- Use `bitbake gpio-led-ctrl -c cleansstate` then build again

### 4.3. Build Complete Image

```bash
# Build image with all components
bitbake core-image-minimal

# Check packages are in image
bitbake core-image-minimal -e | grep ^IMAGE_INSTALL=
cat tmp/deploy/images/raspberrypi0-wifi/core-image-minimal-raspberrypi0-wifi.manifest | grep -E "^(gpio-led-ctrl|kernel-module-gpio-led-driver)"
```

**Expected output:**
```
kernel-module-gpio-led-driver-5.4.72 raspberrypi0_wifi 5.4.72+git0+5d52d9eea9_154de7bbd5
gpio-led-ctrl arm1176jzfshf_vfp 1.0
```

## Part 5: Flash and Test

### 5.1. Flash Image to SD Card

```bash
# Go to image folder
cd tmp/deploy/images/raspberrypi0-wifi/

# Find SD card device
lsblk

# Flash image (replace sdX with real device)
sudo dd if=core-image-minimal-raspberrypi0-wifi.rpi-sdimg of=/dev/sdX bs=4M status=progress conv=fsync
sudo sync
```

### 5.2. Boot and Test on Pi Zero W

```bash
# Connect with SSH
ssh root@<pi-ip>

# Check kernel module is loaded
lsmod | grep gpio_led_driver

# Check device file exists
ls -la /dev/gpio_led

# Test basic commands
gpio_led_ctrl status
gpio_led_ctrl on
gpio_led_ctrl off
gpio_led_ctrl toggle
gpio_led_ctrl blink 3

# Test advanced commands (IOCTL)
gpio_led_ctrl getpin
gpio_led_ctrl ion         # Turn on via ioctl
gpio_led_ctrl istate      # Get state via ioctl
gpio_led_ctrl ioff        # Turn off via ioctl

# Test GPIO pin change
gpio_led_ctrl setpin 27   # Change to GPIO 27
gpio_led_ctrl getpin      # Verify pin change
gpio_led_ctrl on          # Test new pin

# Check kernel messages
dmesg | grep GPIO_LED
```

**Expected results:**
```
# lsmod | grep gpio_led_driver
gpio_led_driver        16384  0

# ls -la /dev/gpio_led
crw------- 1 root root 245, 0 Jan  1 00:00 /dev/gpio_led

# gpio_led_ctrl on
LED turned ON

# gpio_led_ctrl status
GPIO_LED: ON (GPIO18)

# gpio_led_ctrl getpin
Current GPIO pin: 18

# gpio_led_ctrl ion
LED turned ON (via ioctl)
```

# Application Usage Guide

## Basic Commands

### LED Control
```bash
gpio_led_ctrl on              # Turn LED ON
gpio_led_ctrl off             # Turn LED OFF
gpio_led_ctrl toggle          # Toggle LED state
gpio_led_ctrl status          # Show LED status
```

### Blinking
```bash
gpio_led_ctrl blink           # Blink 5 times (default)
gpio_led_ctrl blink 10        # Blink 10 times
gpio_led_ctrl blink 5 200     # Blink 5 times with 200ms delay
```

## Advanced Commands (IOCTL Interface)

### GPIO Pin Management
```bash
gpio_led_ctrl getpin          # Get current GPIO pin
gpio_led_ctrl setpin 27       # Set GPIO pin to 27
gpio_led_ctrl setpin 18       # Set GPIO pin to 18
```

### Alternative LED Control
```bash
gpio_led_ctrl ion             # Turn LED ON (via ioctl)
gpio_led_ctrl ioff            # Turn LED OFF (via ioctl)
gpio_led_ctrl istate          # Get LED state (via ioctl)
```

## Usage Examples

### Basic LED Control
```bash
# Simple on/off control
gpio_led_ctrl on
sleep 2
gpio_led_ctrl off

# Check status
gpio_led_ctrl status
```

### Multi-GPIO Control
```bash
# Control different LEDs
gpio_led_ctrl setpin 18       # Switch to LED on GPIO18
gpio_led_ctrl on              # Turn on LED1

gpio_led_ctrl setpin 27       # Switch to LED on GPIO27
gpio_led_ctrl blink 3         # Blink LED2

gpio_led_ctrl setpin 22       # Switch to LED on GPIO22
gpio_led_ctrl toggle          # Toggle LED3
```

### Hardware Testing
```bash
# Test all available GPIO pins
for pin in 2 3 4 17 18 27 22; do
    echo "Testing GPIO$pin"
    gpio_led_ctrl setpin $pin
    gpio_led_ctrl on
    sleep 1
    gpio_led_ctrl off
done
```

# Control Interfaces

## 1. Read/Write Interface

### Direct Device Access
```bash
# Turn LED ON
echo "1" > /dev/gpio_led

# Turn LED OFF
echo "0" > /dev/gpio_led

# Toggle LED
echo "t" > /dev/gpio_led

# Get LED status
cat /dev/gpio_led
```

### From C Code
```c
int fd = open("/dev/gpio_led", O_WRONLY);
write(fd, "1", 1);  // Turn ON
write(fd, "0", 1);  // Turn OFF
write(fd, "t", 1);  // Toggle
close(fd);
```

## 2. IOCTL Interface

### Purpose of IOCTL
- **Runtime GPIO pin configuration**
- **Alternative control interface** 
- **Structured parameter passing**
- **Advanced features not possible with read/write**

### IOCTL Commands
```c
#define GPIO_LED_IOC_SET_PIN    _IOW('g', 1, int)  // Set GPIO pin
#define GPIO_LED_IOC_GET_PIN    _IOR('g', 2, int)  // Get GPIO pin  
#define GPIO_LED_IOC_SET_STATE  _IOW('g', 3, int)  // Set LED state
#define GPIO_LED_IOC_GET_STATE  _IOR('g', 4, int)  // Get LED state
```

### From C Code
```c
int fd = open("/dev/gpio_led", O_RDWR);

// Change GPIO pin
int pin = 27;
ioctl(fd, GPIO_LED_IOC_SET_PIN, &pin);

// Get current pin
int current_pin;
ioctl(fd, GPIO_LED_IOC_GET_PIN, &current_pin);

// Set LED state
int state = 1;  // ON
ioctl(fd, GPIO_LED_IOC_SET_STATE, &state);

// Get LED state
int led_state;
ioctl(fd, GPIO_LED_IOC_GET_STATE, &led_state);

close(fd);
```

# Common Problems and Solutions

## Problem 1: GPIO Already in Use

**Issue:** 
```
GPIO_LED: Failed to request GPIO18: -16
```

**Solution:**
```bash
# Check what's using the GPIO
cat /sys/kernel/debug/gpio

# Free GPIO if stuck
echo 18 > /sys/class/gpio/unexport

# Or use different pin
gpio_led_ctrl setpin 27
```

## Problem 2: Permission Denied

**Issue:** 
```
Failed to open GPIO LED device: Permission denied
```

**Solution:**
```bash
# Check device permissions
ls -la /dev/gpio_led

# Fix permissions
chmod 666 /dev/gpio_led

# Or run as root
sudo gpio_led_ctrl on
```

## Problem 3: Module Not Loading

**Issue:** Module not found or not loading automatically

**Solution:**
```bash
# Manual module load
modprobe gpio_led_driver

# Check module info
modinfo gpio_led_driver

# Check kernel messages
dmesg | grep GPIO_LED
```

## Problem 4: Multiple IMAGE_INSTALL_append Lines

**Issue:** 
```bash
IMAGE_INSTALL_append = " linux-firmware-bcm43430 wpa-supplicant dhcpcd"
IMAGE_INSTALL_append = " kernel-modules gpio-led-ctrl"
```

**Solution:**
```bash
# Put everything in 1 line
IMAGE_INSTALL_append = " linux-firmware-bcm43430 wpa-supplicant dhcpcd kernel-modules gpio-led-ctrl"

# Or use +=
IMAGE_INSTALL += " linux-firmware-bcm43430 wpa-supplicant dhcpcd"
IMAGE_INSTALL += " kernel-modules gpio-led-ctrl"
```

## Problem 5: Executable Not Created

**Issue:** `find tmp/work/ -name "gpio_led_ctrl" -executable` shows nothing

**Solution:**
```bash
# Force compile step
bitbake gpio-led-ctrl -c compile -f

# Or clean state
bitbake gpio-led-ctrl -c cleansstate
bitbake gpio-led-ctrl
```

# Development Workflow

## When changing kernel driver:

```bash
# 1. Extract kernel source
devtool modify virtual/kernel

# 2. Edit driver code
cd workspace/sources/linux-raspberrypi/drivers/gpio_led_driver
# Edit files...

# 3. Save changes and update patch
git add .
git commit -m "Update GPIO driver"
git format-patch -1 --force
cp *.patch meta-gpio-layer/recipes-kernel/linux/files/

# 4. Reset and rebuild
devtool reset virtual/kernel
bitbake virtual/kernel
bitbake core-image-minimal
```

## When changing userspace app:

```bash
# 1. Edit files in meta-layer
cd meta-gpio-layer/recipes-apps/gpio-led-ctrl/files/
# Edit gpio_led_ctrl.c...

# 2. Rebuild
bitbake gpio-led-ctrl -c cleansstate
bitbake gpio-led-ctrl
bitbake core-image-minimal
```

# Technical Details

## GPIO Interface Used

This project uses the **Linux GPIO subsystem** with these main functions:

- `gpio_request()` - Request GPIO pin from kernel
- `gpio_direction_output()` - Configure pin as output
- `gpio_set_value()` - Set pin HIGH or LOW
- `gpio_get_value()` - Read pin state
- `gpio_free()` - Release GPIO pin

## Driver Features

- **Character device interface** - Standard Linux device model
- **Mutex protection** - Thread-safe operations
- **Error handling** - Proper error codes and cleanup
- **Runtime configuration** - Change GPIO pin without module reload
- **IOCTL support** - Advanced control interface
- **Auto-loading** - Module loads automatically at boot

## Safety Features

- **GPIO conflict detection** - Prevents multiple drivers using same pin
- **Proper cleanup** - Frees resources on module unload
- **Error validation** - Checks parameters before use
- **Kernel logging** - Detailed debug information

# Important Notes

## When to Use This Project:

- **Learning GPIO programming** - Safe and educational
- **Production systems** - Stable and well-tested
- **Multi-platform projects** - Works on different Linux boards
- **Team development** - Easy to understand and maintain

## GPIO Pin Compatibility:

- **Default**: GPIO18 (Pin 12)
- **Tested**: GPIO2, GPIO3, GPIO4, GPIO17, GPIO18, GPIO27, GPIO22
- **Avoid**: GPIO14, GPIO15 (UART), GPIO28-31 (internal)
