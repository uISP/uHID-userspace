/*
*  uHID Universal MCU Bootloader. App loader tool.
*  Copyright (C) 2016  Andrew 'Necromant' Andrianov
*
*  This file is part of uHID project. uHID is loosely (very)
*  based on bootloadHID avr bootloader by Christian Starkjohann
*
*  uHID is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*
*  uHID is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with uHID.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LIBUHID_H
#define LIBUHID_H

#define UISP_PART_NAME_LEN  8
#include <stdint.h>
#include <hidapi/hidapi.h>

struct uHidDeviceMatch {
	int vendor;
	int product;
	wchar_t *vendorName;
	wchar_t *productName;
	wchar_t *serialNumber;
};

struct uHidPartInfo {
	uint16_t      pageSize;
	uint32_t      size;
	uint8_t       ioSize;
	uint8_t       name[UISP_PART_NAME_LEN];
}  __attribute__((packed));

struct uHidDeviceInfo {
	uint8_t       numParts;
	uint8_t       cpuFreq;
	struct uHidPartInfo parts[];
} __attribute__((packed));

#include "uhid_export_glue.h"

UHID_API struct uHidDeviceInfo *uhidReadInfo(hid_device *dev);
UHID_API hid_device *uhidOpen(struct uHidDeviceMatch *deviceMatch);
UHID_API char *uhidReadPart(hid_device *dev, int part, int *bytes_read);
UHID_API int uhidWritePart(hid_device *dev, int part, const char *buf, int length);
UHID_API void uhidClose(hid_device *dev);
UHID_API void uhidCloseAndRun(hid_device *dev, int part);
UHID_API void uhidPrintInfo(struct uHidDeviceInfo *inf);
UHID_API struct hid_device_info *uhidListDevices(struct uHidDeviceMatch *deviceMatch);
UHID_API void uhidProgressCb(void (*cb)(const char *label, int cur, int max));
UHID_API int uhidWritePartFromFile(hid_device *dev, int part, const char *filename);
UHID_API int uhidReadPartToFile(hid_device *dev, int part, const char *filename);
UHID_API int uhidVerifyPart(hid_device *dev, int part, const char *buf, int len);
UHID_API int uhidVerifyPartFromFile(hid_device *dev, int part, const char *filename);
UHID_API int uhidLookupPart(hid_device *dev, const char *name);

#endif
