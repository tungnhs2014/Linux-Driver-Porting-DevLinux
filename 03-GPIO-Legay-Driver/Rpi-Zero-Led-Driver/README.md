# LED Driver for Raspberry Pi Zero W

## Overview

This project creates a complete system to control LED on Raspberry Pi Zero W with:

- **Kernel driver**: Character device driver with direct register access to BCM2835 GPIO
- **Userspace application**: Command-line tool to talk with driver through `/dev/led`
- **Yocto integration**: Complete meta-layer to build driver and app into Linux image

**Main Features:**
- Character device `/dev/led` to control LED
- Direct GPIO register access (no Linux GPIO subsystem)
- Commands: on/off/status/blink with custom parameters
- Auto-load kernel module at boot
- Full integration with Yocto Project build system

## System Architecture

```
┌─────────────────┐    ┌──────────┐    ┌───────────────┐    ┌──────────────┐
│ User Commands   │◄──►│ /dev/led │◄──►│ Kernel Driver │◄──►│ GPIO18 (LED) │
│ led_control on  │    │ (chardev)│    │ (led_driver)  │    │   Hardware   │
└─────────────────┘    └──────────┘    └───────────────┘    └──────────────┘
```

## Hardware Setup

```
Pi Zero W Pin 11 (GPIO18) ──[330Ω Resistor]──[LED Anode]
Pi Zero W Pin 9  (GND)     ────────────────────[LED Cathode]
```

## Project Structure

```
rpi-zero-led-project/
├── README.md
├── kernel-module/
│   ├── led_driver.c          # Kernel driver source
│   ├── led_driver.h          # Driver header  
│   ├── Makefile              # Driver Makefile
│   └── Kconfig               # Kernel config
├── userspace-app/
│   ├── led_control.c         # App source
│   ├── led_control.h         # App header
│   └── Makefile              # App Makefile
└── meta-led-layer/           # Yocto meta layer
    ├── conf/
    │   └── layer.conf
    ├── recipes-kernel/
    │   └── linux/
    │       ├── linux-raspberrypi_%.bbappend
    │       └── linux-raspberrypi/
    │           └── 0001-Add-LED-driver-for-Pi-Zero-W.patch
    └── recipes-apps/
        └── led-control/
            ├── led-control_1.0.bb
            └── files/
                ├── led_control.c
                ├── led_control.h
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
mkdir ~/yocto-led-build
cd ~/yocto-led-build

# Clone Yocto and layers
git clone git://git.yoctoproject.org/poky -b dunfell
git clone git://git.yoctoproject.org/meta-raspberrypi -b dunfell

# Copy meta-layer to workspace
cp -r ~/rpi-zero-led-project/meta-led-layer .

# Setup build environment
cd poky
source oe-init-build-env
```

### 1.3. Configure Layers

```bash
# Add layers
bitbake-layers add-layer ../../meta-raspberrypi
bitbake-layers add-layer ../../meta-led-layer

# Check layers
bitbake-layers show-layers
```

## Part 2: Create Kernel Patch (DevTool Method)

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

### 2.2. Add LED Driver to Kernel Source Tree

```bash
# Create folder for driver
mkdir -p drivers/led_driver

# Copy source files from project
cp ~/rpi-zero-led-project/kernel-module/* drivers/led_driver/

# Update kernel build system
echo 'obj-$(CONFIG_LED_DRIVER) += led_driver/' >> drivers/Makefile
echo 'source "drivers/led_driver/Kconfig"' >> drivers/Kconfig

# Add config to defconfig (loadable module)
echo 'CONFIG_LED_DRIVER=m' >> arch/arm/configs/bcmrpi_defconfig
```

### 2.3. Commit and Create Patch

```bash
# Add files to git
git add drivers/led_driver/
git add drivers/Makefile
git add drivers/Kconfig
git add arch/arm/configs/bcmrpi_defconfig

# Save changes
git commit -m "Add LED character device driver for Pi Zero W

# Create patch file
git format-patch -1

# Copy patch to meta-layer
cp 0001-*.patch ~/yocto-led-build/meta-led-layer/recipes-kernel/linux/files/
```

### 2.4. Reset DevTool Workspace

```bash
# Go back to build folder
cd ~/yocto-led-build/poky/build

# Reset devtool (switch from development to production mode)
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

# LED Project packages (IMPORTANT: Only 1 IMAGE_INSTALL_append line)
IMAGE_INSTALL_append = " linux-firmware-bcm43430 wpa-supplicant dhcpcd kernel-modules led-control"

# Development features
EXTRA_IMAGE_FEATURES += "ssh-server-dropbear debug-tweaks"

# Image format
IMAGE_FSTYPES = "tar.xz ext4 rpi-sdimg"

# Build speed up
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
cp ~/rpi-zero-led-project/userspace-app/* \
   ~/yocto-led-build/meta-led-layer/recipes-apps/led-control/files/
```

## Part 4: Build Process

### 4.1. Build Kernel with Driver

```bash
cd ~/yocto-led-build/poky/build

# Build kernel
bitbake virtual/kernel

# Check kernel module is built
find tmp/work/ -name "led_driver.ko"
```

### 4.2. Build Userspace Application

```bash
# Build LED control app
bitbake led-control

# If compile error happens, force rebuild:
bitbake led-control -c compile -f

# Check executable is created
find tmp/work/ -name "led_control" -executable
```

**Troubleshooting:** If executable not found:
- Build process might be stuck due to Yocto cache
- Use `bitbake led-control -c cleansstate` then build again

### 4.3. Build Complete Image

```bash
# Build image with all components
bitbake core-image-minimal

# Check packages are in image
bitbake core-image-minimal -e | grep ^IMAGE_INSTALL=
cat tmp/deploy/images/raspberrypi0-wifi/core-image-minimal-raspberrypi0-wifi.manifest | grep -E "^(led-control|kernel-module-led-driver)"
```

**Expected output:**
```
kernel-module-led-driver-5.4.72 raspberrypi0_wifi 5.4.72+git0+5d52d9eea9_154de7bbd5
led-control arm1176jzfshf_vfp 1.0
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
lsmod | grep led_driver

# Check device file exists
ls -la /dev/led

# Test userspace application
led_control status
led_control on
led_control off
led_control blink 3
led_control blink 5 1000

# Check kernel messages
dmesg | grep LED
```

**Expected results:**
```
# lsmod | grep led_driver
led_driver             16384  0

# ls -la /dev/led
crw------- 1 root root 245, 0 Jan  1 00:00 /dev/led

# led_control on
LED turned ON

# led_control status
LED: ON (GPIO18)
```

# Common Problems and Solutions

## Problem 1: Multiple IMAGE_INSTALL_append Lines

**Issue:** 
```bash
IMAGE_INSTALL_append = " linux-firmware-bcm43430 wpa-supplicant dhcpcd"
IMAGE_INSTALL_append = " kernel-modules led-control"
```

**Solution:**
```bash
# Put everything in 1 line
IMAGE_INSTALL_append = " linux-firmware-bcm43430 wpa-supplicant dhcpcd kernel-modules led-control"

# Or use +=
IMAGE_INSTALL += " linux-firmware-bcm43430 wpa-supplicant dhcpcd"
IMAGE_INSTALL += " kernel-modules led-control"
```

## Problem 2: Files Not Found in Fetch/Unpack

**Issue:** Yocto cannot find source files

**Solution:**
```bash
# Check files exist
ls -la meta-led-layer/recipes-apps/led-control/files/

# Force rebuild
bitbake led-control -c cleansstate
bitbake led-control
```

## Problem 3: Executable Not Created

**Issue:** `find tmp/work/ -name "led_control" -executable` shows nothing

**Solution:**
```bash
# Force compile step
bitbake led-control -c compile -f

# Or clean state
bitbake led-control -c cleansstate
bitbake led-control
```

## Problem 4: GNU_HASH Warning (QA Issue)

**Issue:** 
```
QA Issue: File /usr/bin/led_control doesn't have GNU_HASH (didn't pass LDFLAGS?)
```

**Solution:**
Add to recipe `led-control_1.0.bb`:
```bash
INSANE_SKIP_${PN} = "ldflags"
```

## Problem 5: Packages Not in Image

**Issue:** Packages are built but not in image

**Solution:**
- Check IMAGE_INSTALL syntax in local.conf
- Rebuild image after fixing local.conf
- Check with manifest file

# Development Workflow

## When changing kernel driver:

```bash
# 1. Extract kernel source
devtool modify virtual/kernel

# 2. Edit driver code
cd workspace/sources/linux-raspberrypi/drivers/led_driver
# Edit files...

# 3. Save changes and update patch
git add .
git commit -m "Update driver"
git format-patch -1 --force
cp *.patch meta-led-layer/recipes-kernel/linux/files/

# 4. Reset and rebuild
devtool reset virtual/kernel
bitbake virtual/kernel
bitbake core-image-minimal
```

## When changing userspace app:

```bash
# 1. Edit files in meta-layer
cd meta-led-layer/recipes-apps/led-control/files/
# Edit led_control.c...

# 2. Rebuild
bitbake led-control -c cleansstate
bitbake led-control
bitbake core-image-minimal
```