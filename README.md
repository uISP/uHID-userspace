[![Build Status](https://jenkins.ncrmnt.org/job/GithubCI/job/uhid/badge/icon)](https://jenkins.ncrmnt.org/job/GithubCI/job/uhid/)

# WUT?

uHID is the Universal HID bootloader for various MCUs. E.g. it gets YOUR code
into YOUR MCU over the USB bus when you flash the bootloader. This repository
contains the userspace tools only.

It takes the well-known bootloadHID idea of wrapping bootloading process in HID
protocol and takes it one step further

# Why is it awesome?

- uHID makes itself appear as a usb HID device. Windows users need no driver, it
just works out of the box, that's especially useful when driver signing has become
obligatory

- uHID provides a single and simple SPEC for writing bootloaders over USB. If your
own bootloader code follows it - you can use all the awesome userspace apps it
provides.

- uHID allows chips to export a set of 'partitions', therefore MCUs with more
horsepower may load several apps.

- uHID userspace app that sends the code to your MCU is available for windows,
linux and mac.

- uHID provides all it's low-level functions is a small C library. That makes it
easy to integrate in your own apps.

- uHID is TESTED: Every commit is built for windows, linux and mac and tested
against REAL hardware.

- uHID is checked vith coveralls and clang-analyze

- uHID userspace code is LGPLv2, you can link it against your own apps.

# What MCUs are supported
Currently AVR and nRF24LU1 are supported, if you want more - contribute!

# Download binaries

ZIP archives contain a static binary that doesn't need anything
else to run. Just drop uhidtool[.exe] somewhere in your PATH and you are
good to go.
Mac and Linux users should also do a chmod +x on the binary, e.g.

```
unzip pkg.zip
cd pkg
sudo cp bin/uhidtool /usr/local/bin
sudo chmod +x /usr/local/bin/uhidtool
```

TODO: deb packages

# Compiling from source

## Linux (sudo make install)

You'll need cmake, pkg-config, libhidapi-dev and libhidapi-libusb0 packages.
On debian/ubuntu just run the following:
```
sudo apt-get install build-essential cmake pkg-config libhidapi-dev libhidapi-libusb0
```

Next compile uHID
```
mkdir build
cd build
cmake ..
make
sudo make install
```

## Linux (Debian package)

You'll need dependencies, just like the above to build your packages

```
debuild -us -uc
```

## Windows

VS C Compiler is not supported! Contributions welcome!
Windows builds are tricky, since windows lacks pkg-config as of 2016 (sic!).

###First, you'll have to install build tools by hand.

- cmake: https://cmake.org/download/

- MinGW: http://www.mingw.org/ (at least the compiler and mingw32-make)

- Git For Windows: https://git-for-windows.github.io/

### Add your MinGW bin dir to %PATH%, e.g. C:\\MinGW\\bin

See: My PC -> Properties -> Advanced -> Environment Variables

### Get uHID sourcecode

Either via zip from git hub or using git clone.

### Download hidapi and unpack to uHID_DIR/deps/hidapi

You can grab hidapi here: https://github.com/signal11/hidapi

### Open CMD prompt and change dir to your uHID source code

```
C:
cd C:\uHID
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=StaticRelease
mingw32-make
mingw32-make package
```

At this point you'll have a zip with all the files.
-DCMAKE_BUILD_TYPE=StaticRelease make the resulting binary standalone
so that you can just drop it somewhere convenient and use. The default behavior
is to dynamically link against libuhid and mingw, which is not something you
normally want on windows

## MAC

### Install commandline developer tools. Just opening the terminal and typing
```
gcc
```
will prompt you to do so (At least on Yosemite)

### Install cmake from https://cmake.org/download

### Add cmake to your path

```
sudo mkdir -p /usr/local/bin
sudo /Applications/CMake.app/Contents/bin/cmake-gui --install=/usr/local/bin
```

### Get uHID sourcecode

Either via zip from git hub or using git clone.

```
cd /path/to/uhid/source
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=StaticRelease
mingw32-make
mingw32-make package
```

### At this point you will end up with a shiny zip package

-DCMAKE_BUILD_TYPE=StaticRelease make the resulting binary standalone
so that you can just drop it to bin/ and use. The default behavior
is to dynamically link against libuhid.

# The commandline interface

uHID comes with 2 commandline tools.
- uhidtool is the raw read/write/run tool for low-level operations on a device
- uhidpkg is a 'package'-manager that allows you to manage a local repository of images for your uHID-based devices and swap between them seamlessly  

At the time of writing uhidpkg is under heavy develoment.

uHID bootloader tool (c) Andrew 'Necromant' Andrianov 2016
This is free software subject to GPLv2 license.

```
Usage:
uhidtool --help                      - This help message
uhidtool --info                      - Show info about device
uhidtool --crc                       - Calculate and display partition CRC
uhidtool --part eeprom --write 1.bin - Write partition eeprom with 1.bin
uhidtool --part eeprom --read  1.bin - Read partition eeprom to 1.bin
uhidtool --run [flash]               - Execute code in partition [flash]
                                 Optional, if supported by target MCU

uHIDtool can read intel hex as well as binary.
The filename extension should be .ihx or .hex for it to work
```

# The SPEC

## Overview

The data is trasfered between the PC and the MCU using HID feature reports.
Every chunk of data must have one byte report id just before the actual data you are going to handle and this has to be accounted for on the MCU side.

## USB device descriptors and The Great VID/PID ID Mess.

USB.org sells ~~air~~ VID/PID pairs at a huge price ~5k$ (A huge load of money to waste on some 'number'). All hobbyists are politely asked to go to hell.
Here's an example of how they treat opensource:

http://www.arachnidlabs.com/blog/2013/10/18/usb-if-no-vid-for-open-source/

The uHID bootloader userspace tools have a table of compatible devices built-in. At the time of writing it looks like this (serial number and product name have a special meaning, see below!). These can be overriden via commandline or API.

```
static struct uHidDeviceMatch compatibleDevices[] = {
		{
			/* Necromant's VID/PID for bootloading */
			.vendor  = 0x1d50,
		    .product = 0x6032,
			.vendorName = L"uHID",
	  },
		{
			/* obdev.at bootloadHID VID/PID */
			.vendor = 0x16c0,
			.product = 0x05df,
			.vendorName = L"uhid.ncrmnt.org",
		},
			/* Proto
		{
			.vendor = 0x6666,
			.product = 0x6666,
		}
		{ /* Sentinel */ }
};
```

If you want to use obdev.at's bootloadHID VID/PID pair - see their vusb license
for details! In a nutshell - your product should be opensource/openhardware and licensed under their modified GPLv2.

If you want to use my (Necromant's) VID/PID that I have obtained from the awesome OpenMoko people when they were handing those out for free, roughly the same stuff applies.
See: http://wiki.openmoko.org/wiki/USB_Product_IDs#Open_registry_for_community_.2F_homebrew_USB_Product_IDs

If you want to use those IDs for the uHID bootloader in your device you MUST follow the following rules:

1. The Vendor Name must be set to uHID or uhid.ncrmnt.org matching the ids, as indicated above

2. Product Name must match the microcontroller name. E.g. atmega8 for atmega8.
Try to use the same name that you supply compilers and applications like gcc, avrdude, etc. This will be useful to have when running scripts

3. Serial Number is basically your device name + serial number, whatever. You can use it to chose between different connected devices and use : as a separator
e.g. _MyDevKit:first_, _MyDevKit:second_ etc. App manager recognises : and handles it properly

That said, if you decide for your project that uses uHID to go commercial, you have to:

1. Make sure the actual bootloader license allows it. Avr vusb bootloader

2. Obtain your own vid/pid device ids

3. Send me a pull request so that I can add your device ids to the userspace app
if you feel like

4. Make sure you link with uHID userspace lib dynamically, so that you follow the LGPLv2 text

## The HID reports descriptor
That said, for this to work your device MUST have proper HID report descriptors. Here's an example of HID descriptors for AVR (with 2 partitions: 'flash' and 'eeprom')

```
/* Maximum IO size (without report id byte) */
/* Make sure that you allocate MAXIOSIZE+1 bytes buffer
   in MCU firmware */
#define MAXIOSIZE 64

const char usbHidReportDescriptor[42] = {
	0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
	0x09, 0x01,                    // USAGE (Vendor Usage 1)
	0xa1, 0x01,                    // COLLECTION (Application)
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
	0x75, 0x08,                    //   REPORT_SIZE (8)

   /* Device information report / app exec  */
	0x85, 0x01,                    //   REPORT_ID (1)
	0x95, MAXIOSIZE,                    //   REPORT_COUNT (MAXIOSIZE)
	0x09, 0x00,                    //   USAGE (Undefined)
	0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

   /* Flash  */
	0x85, 0x02,                    //   REPORT_ID (2)
	0x95, MAXIOSIZE,                    //   REPORT_COUNT (MAXIOSIZE)
	0x09, 0x00,                    //   USAGE (Undefined)
	0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

   /* eeprom */
	0x85, 0x03,                    //   REPORT_ID (3)
	0x95, MAXIOSIZE,                    //   REPORT_COUNT (MAXIOSIZE)
	0x09, 0x00,                    //   USAGE (Undefined)
	0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

	0xc0                           // END_COLLECTION
};
```

The number of reports in the collection must be the number of partitions + 1.
The numbering starts with 1, partitions start from the second report.

Windows usb HID API is very strict and the report size must match the actual chunks of data being trasfered over that report (Minus the report id byte)
In the example above flash and eeprom data is sent to the MCU 65 byte at a time
with 64 actual bytes of data and 1 byte report Id.

## Report ID 1 (Read)

When report id 1 is read from the device it should contain an informational structure called deviceInfo followed by a set of partInfo structs.

_WARNING_: Reading informational report from device should also reset the internal address pointer for read and write operations.

Here's how they are defined example:
```
struct partInfo {
	uint16_t      pageSize;
	uint32_t      size;
	uint8_t       ioSize;
	uint8_t       name[UISP_PART_NAME_LEN];
}  __attribute__((packed));

struct deviceInfo {
	uint8_t       version;
	uint8_t       numParts;
	uint8_t       cpuFreq;
	struct partInfo parts[];
} __attribute__((packed));
```

And here's the definition for AVRs:

```

static const PROGMEM struct deviceInfo devInfo = {
	.version = 1,
	.cpuFreq = F_CPU / 100000,
	.numParts = 2,
	.parts = {
		{SPM_PAGESIZE, CONFIG_AVR_BLDADDR, IOBUFLEN,       "flash"  },
		{SPM_PAGESIZE, E2END + 1,          IOBUFLEN,       "eeprom"}
	},
};
```

A few notes on fields of the structs:

### version

Stands for informational structs version. Currently it's 1.

### cpuFreq

Stands for CPU frequency in (Hz / 100000) units.

### numParts

The number of partitins the device contains

### parts
The array of actual partitions, matching the count above

#### pageSize
The 16-bit page sizes of the memory device. The uploaded files will be padded with zeroes to the next page boundary by the userspace tools

#### ioSize
The 8-bit value defines how much data each sent report contains. This must match the HID report descriptor above or it won't work in windows. Normally you'd want (pageSize % ioSize) == 0  

#### size
The 32-bit size of the partition in bytes

#### name
The up-to-eight character string name of the partition.

## Report ID 1 (Write)
Writing to this report should be treated as a command to start the application loaded in the MCU. If the device has multiple partitions that can execute code the report shall contain exectly one data byte with the number of the partition to boot

## Report ID 2+n (Reading and writing partitions)
Reading or writing to a partition is achieved in the dumbest way possible. By reading/writing a report with number 2 + the number of partition we have to work on. The device should have an internal address pointer that should be incremented after every read and write operation

Reading the informational report as described above or resetting the device should reset this pointer to 0.

Each report contains a 1-byte report-id + the data that we want to write to
the partition.

Each report that is read contains a one-byte report-id + tha data we've read.

Every next report reads/writes the next data batch

# Authors

Andrew 'Necromant' Andrianov <www.ncrmnt.org>
