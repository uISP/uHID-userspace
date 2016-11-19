[![Build Status](https://jenkins.ncrmnt.org/job/GithubCI/job/uhid/badge/icon)](https://jenkins.ncrmnt.org/job/GithubCI/job/uhid/)

# N.B.

This is the placeholder README for now.

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

- TODO

# Compiling

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
cmake .. -G "MinGW Makefiles"
mingw32-make
mingw32-make package
```

At this point you'll have a zip with all the files. The uhidtool is linked
statically, so it doesn't require much to run on a fresh system


## MAC

TODO


# The SPEC

TODO: Loader spec here

# Authors

Andrew 'Necromant' Andrianov <contact [at] ncrmnt [dot] org>
