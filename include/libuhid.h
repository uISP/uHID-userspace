/*
 *  uHID Universal MCU Bootloader. Library.
 *  Copyright (C) 2016  Andrew 'Necromant' Andrianov
 *
 *  This file is part of uHID project. uHID was initially based
 *  on bootloadHID avr bootloader by Christian Starkjohann
 *  Since no original userspace code remains, all userspace code
 *  is now LGPLv2.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.

 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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


/*
	name: usbasp
	version: 1.0
	description: UsbASP avr programmer firmware for uISP
	maintainer: Necromant <spam at ncrmnt dot org>
	map: fw.hex->flash
	map: fw.eep->eeprom
 */

struct uhidAppPartMap {
	char *file;
	char *part;
	struct uhidAppPartMap *next;
};

struct uhidApplication {
	char *name;
	char *version;
	char *description;
	char *maintainer;
	struct uhidAppPartMap *map;
};

struct uhidRepositoryHandle {
	struct dirent *dir;
	char *dirpath;
};

struct uHidPartInfo {
	uint16_t      pageSize;
	uint32_t      size;
	uint8_t       ioSize;
	uint8_t       name[UISP_PART_NAME_LEN];
}  __attribute__((packed));

struct uHidDeviceInfo {
    uint8_t       reportId;
    uint8_t       version;
	uint8_t       numParts;
	uint16_t       cpuFreq;
	struct uHidPartInfo parts[];
} __attribute__((packed));

#include "uhid_export_glue.h"

UHID_API struct uHidDeviceInfo *uhidReadInfo(hid_device *dev);
UHID_API hid_device *uhidOpen(struct uHidDeviceMatch *deviceMatch);
UHID_API char *uhidReadPart(hid_device *dev, int part, int *bytes_read);
UHID_API int uhidWritePart(hid_device *dev, int part, const char *buf, int length);
UHID_API void uhidClose(hid_device *dev);
UHID_API int uhidCloseAndRun(hid_device *dev, int part);
UHID_API void uhidPrintInfo(hid_device *dev, struct uHidDeviceInfo *inf);
UHID_API struct hid_device_info *uhidListDevices(struct uHidDeviceMatch *deviceMatch);
UHID_API void uhidProgressCb(void (*cb)(const char *label, int cur, int max));
UHID_API int uhidWritePartFromFile(hid_device *dev, int part, const char *filename);
UHID_API int uhidReadPartToFile(hid_device *dev, int part, const char *filename);
UHID_API int uhidVerifyPart(hid_device *dev, int part, const char *buf, int len);
UHID_API int uhidVerifyPartFromFile(hid_device *dev, int part, const char *filename);
UHID_API int uhidLookupPart(hid_device *dev, const char *name);
UHID_API float uhidGetFrequencyMhz(struct uHidDeviceInfo *i);

UHID_API int uhidGetPartitionCRC(hid_device *dev, const char *part, uint32_t *crc32);
UHID_API int uhidGetPartitionCRCById(hid_device *dev, int part, uint32_t *crc32);

/* Private library stuff */
UHID_NO_EXPORT uint32_t CRC32FromBuf(uint32_t inCrc32, const void *buf,
                                       size_t bufLen );
UHID_NO_EXPORT int CRC32FromFd( FILE *file, uint32_t *outCrc32 );

#endif
