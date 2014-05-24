/* Name: main.c
 * Project: AVR bootloader HID
 * Author: Christian Starkjohann
 * Creation Date: 2007-03-19
 * Tabsize: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: Proprietary, free under certain conditions. See Documentation.
 * This Revision: $Id: main.c 787 2010-05-30 20:54:25Z cs $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "usbcalls.h"

#define IDENT_VENDOR_NUM        0x16c0
#define IDENT_VENDOR_STRING     "www.ncrmnt.org"
#define IDENT_PRODUCT_NUM       1503
#define IDENT_PRODUCT_STRING    "Necromant's uISP"

/* ------------------------------------------------------------------------- */

static char dataBuffer[65536 + 256];    /* buffer for file data */
static int  startAddress, endAddress;
static char leaveBootLoader = 0;

/* ------------------------------------------------------------------------- */

static int  parseUntilColon(FILE *fp)
{
	int c;

	do {
		c = getc(fp);
	} while(c != ':' && c != EOF);
	return c;
}

static int  parseHex(FILE *fp, int numDigits)
{
	int     i;
	char    temp[9];

	for(i = 0; i < numDigits; i++)
		temp[i] = getc(fp);
	temp[i] = 0;
	return strtol(temp, NULL, 16);
}

/* ------------------------------------------------------------------------- */

static int  parseIntelHex(char *hexfile, char buffer[65536 + 256], int *startAddr, int *endAddr)
{
	int     address, base, d, segment, i, lineLen, sum;
	FILE    *input;

	input = fopen(hexfile, "r");
	if(input == NULL) {
		fprintf(stderr, "error opening %s: %s\n", hexfile, strerror(errno));
		return 1;
	}
	while(parseUntilColon(input) == ':') {
		sum = 0;
		sum += lineLen = parseHex(input, 2);
		base = address = parseHex(input, 4);
		sum += address >> 8;
		sum += address;
		sum += segment = parseHex(input, 2);  /* segment value? */
		if(segment != 0)    /* ignore lines where this byte is not 0 */
			continue;
		for(i = 0; i < lineLen ; i++) {
			d = parseHex(input, 2);
			buffer[address++] = d;
			sum += d;
		}
		sum += parseHex(input, 2);
		if((sum & 0xff) != 0) {
			fprintf(stderr, "Warning: Checksum error between address 0x%x and 0x%x\n", base, address);
		}
		if(*startAddr > base)
			*startAddr = base;
		if(*endAddr < address)
			*endAddr = address;
	}
	fclose(input);
	return 0;
}

/* ------------------------------------------------------------------------- */

char    *usbErrorMessage(int errCode)
{
	static char buffer[80];

	switch(errCode) {
	case USB_ERROR_ACCESS:
		return "Access to device denied";
	case USB_ERROR_NOTFOUND:
		return "The specified device was not found";
	case USB_ERROR_BUSY:
		return "The device is used by another application";
	case USB_ERROR_IO:
		return "Communication error with device";
	default:
		sprintf(buffer, "Unknown USB error %d", errCode);
		return buffer;
	}
	return NULL;    /* not reached */
}

static int  getUsbInt(char *buffer, int numBytes)
{
	int shift = 0, value = 0, i;

	for(i = 0; i < numBytes; i++) {
		value |= ((int)*buffer & 0xff) << shift;
		shift += 8;
		buffer++;
	}
	return value;
}

static void setUsbInt(char *buffer, int value, int numBytes)
{
	int i;

	for(i = 0; i < numBytes; i++) {
		*buffer++ = value;
		value >>= 8;
	}
}

/* ------------------------------------------------------------------------- */

typedef struct deviceInfo {
	char          reportId;
	char          pageSize[2];
	char          flashSize[4];
	unsigned char cpu_freq;
} deviceInfo_t  __attribute__((packed));

typedef struct deviceData {
	char    reportId;
	char    address[3];
	char    data[128];
} deviceData_t  __attribute__((packed));

typedef struct eefunc {
	char    reportId;
	char    rw;
	short    address;
	char    data;
} eeData_t  __attribute__((packed));


#define eeprom_write_byte(dev, addr, data)	\
	eeprom_dance(dev, addr, 1, data);

static void eeprom_dance(usbDevice_t *dev, short addr, char rw, char data)
{
	int         err = 0;
	union {
		char            bytes[1];
		deviceInfo_t    info;
		deviceData_t    data;
		eeData_t		ee;
	}           buffer;
	buffer.ee.reportId=3;
	buffer.ee.rw=rw; //set addr
	buffer.ee.data=data;
	buffer.ee.address = addr;
	if((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, buffer.bytes, 6) != 0)) {
		fprintf(stderr, "Error reading info: %s\n", usbErrorMessage(err));
	}
}

static char eeprom_read_byte(usbDevice_t *dev, short addr)
{
	int         err = 0, len;
	union {
		char            bytes[1];
		deviceInfo_t    info;
		deviceData_t    data;
		eeData_t		ee;
	}           buffer;

	eeprom_dance(dev, addr, 0, 0);
	len = sizeof(buffer.ee);
	if((err = usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 3, buffer.bytes, &len)) != 0) {
		fprintf(stderr, "Error reading page size: %s\n", usbErrorMessage(err));
	}

	if(len < sizeof(buffer.ee)) {
		fprintf(stderr, "Not enough bytes in device info report (%d instead of %d)\n", len, (int)sizeof(buffer.ee));
		err = -1;
	}
	return buffer.ee.data;
}

static int eeprom_read_buf(usbDevice_t *dev, short addr, int size, char* buf)
{
	int i = 0;
	while (i<size) {
		buf[i]=eeprom_read_byte(dev, addr);
		addr++;
		i++;
	}
}

static int eeprom_write_buf(usbDevice_t *dev, short addr, int size, char* buf)
{
	int i = 0;
	while (i<size) {
		eeprom_write_byte(dev, addr, buf[i]);
		addr++;
		i++;
	}
}

#define _MMAGIC	0x6F
#define EESZ	512
#define ENDURANCE	100000
#define OFFSET EESZ-sizeof(struct devinfo)

static struct devinfo {
	char magic;
	char app[16];
	char app_version[16];
	int rwcnt;
};


struct devinfo dtest = {
	.magic = 0x6f,
	.app = "usbasp",
	.app_version = "git",
	.rwcnt=100
};

static void update_dev_info(usbDevice_t* dev, char* app, char* appver)
{
	printf("Updated dev info to: %s %s\n", app, appver);
	eeprom_read_buf(dev,OFFSET,sizeof(struct devinfo),&dtest);
	if (dtest.magic!=_MMAGIC) {
		dtest.magic=_MMAGIC;
		dtest.rwcnt=0;
		printf("INFO: This used to be a virgin device\n");
		printf("INFO: Not anymore\n");
	}
	dtest.rwcnt++;
	strncpy(dtest.app,app,16);
	strncpy(dtest.app_version,appver,16);
	eeprom_write_buf(dev,OFFSET,sizeof(struct devinfo),&dtest);
}

static int list_devices()
{
	usbDevice_t *dev = NULL;
	int         err = 0, len, mask, pageSize, deviceSize;
	union {
		char            bytes[1];
		deviceInfo_t    info;
		deviceData_t    data;
		eeData_t		ee;
	}           buffer;

	buffer.ee.reportId=3;
	buffer.ee.rw=0;
	printf("Listing detected uISP devices (if any)\n");
	if((err = usbOpenDevice(&dev, IDENT_VENDOR_NUM, IDENT_VENDOR_STRING, IDENT_PRODUCT_NUM, IDENT_PRODUCT_STRING, 1, 2)) != 0) {
//         fprintf(stderr, "Error opening HIDBoot device: %s\n", usbErrorMessage(err));
		exit(1);
	}
}



static int dump_device_info(char* serial, int cpuonly)
{
	usbDevice_t *dev = NULL;
	int         err = 0, len, mask, pageSize, deviceSize;
	union {
		char            bytes[1];
		deviceInfo_t    info;
		deviceData_t    data;
		eeData_t		ee;
	}           buffer;

	buffer.ee.reportId=3;
	buffer.ee.rw=0;
	if((err = usbOpenDevice(&dev, IDENT_VENDOR_NUM, IDENT_VENDOR_STRING, IDENT_PRODUCT_NUM, IDENT_PRODUCT_STRING, serial, 1)) != 0) {
		fprintf(stderr, "Error opening HIDBoot device: %s\n", usbErrorMessage(err));
		exit(1);
	}
	eeprom_read_buf(dev,OFFSET,sizeof(struct devinfo),&dtest);
    
	len = 8;
	if((err = usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 1, buffer.bytes, &len)) != 0) {
		fprintf(stderr, "Error reading page size: %s\n", usbErrorMessage(err));
		exit(1);
	}
    
    
	if (!buffer.info.cpu_freq) 
	{
		buffer.info.cpu_freq = 120;
		fprintf(stderr, "Warning! Please update your bootloader on the device!\n");
	}

	if (cpuonly) { 
		printf("%0.1f\n", buffer.info.cpu_freq * 100000 / 1000000.0);
		return 0;
	}
	printf("Necromant's uISP Tool\n");
	if (dtest.magic!=_MMAGIC) {
		printf("No valid magic found. Virgin device? \n");
		printf("A valid info struct will be written once we load an app for the first time\n");
		return 0;
	} else {
		float health = 100.0-((float) 100.0/ENDURANCE ) * dtest.rwcnt;
		printf("Device health:\t%f%% (%d/%d)\n", health, dtest.rwcnt, ENDURANCE);
		printf("Application:\t%s\n", dtest.app);
		printf("App version:\t%s\n", dtest.app_version);
		printf("CPU Frequency:\t%.01f Mhz\n", buffer.info.cpu_freq * 100000 / 1000000.0);
		return 0;
	}
}

static int uploadData(char *dataBuffer, int startAddr, int endAddr, char* app_name, char* app_version, char* serial)
{
	usbDevice_t *dev = NULL;
	int         err = 0, len, mask, pageSize, deviceSize;
	union {
		char            bytes[1];
		deviceInfo_t    info;
		deviceData_t    data;
	}           buffer;

	if((err = usbOpenDevice(&dev, IDENT_VENDOR_NUM, IDENT_VENDOR_STRING, IDENT_PRODUCT_NUM, IDENT_PRODUCT_STRING, serial, 1)) != 0) {
		fprintf(stderr, "Error opening HIDBoot device: %s\n", usbErrorMessage(err));
		goto errorOccurred;
	}
	len = sizeof(buffer);
	if(endAddr > startAddr) {    // we need to upload data
		printf("Upload dance...\n");
		if((err = usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 1, buffer.bytes, &len)) != 0) {
			fprintf(stderr, "Error reading page size: %s\n", usbErrorMessage(err));
			goto errorOccurred;
		}
		if(len < sizeof(buffer.info)) {
			fprintf(stderr, "Not enough bytes in device info report (%d instead of %d)\n", len, (int)sizeof(buffer.info));
			err = -1;
			goto errorOccurred;
		}
		pageSize = getUsbInt(buffer.info.pageSize, 2);
		deviceSize = getUsbInt(buffer.info.flashSize, 4);
		printf("Page size   = %d (0x%x)\n", pageSize, pageSize);
		printf("Device size = %d (0x%x); %d bytes remaining\n", deviceSize, deviceSize, deviceSize - 2048);
		if(endAddr > deviceSize - 2048) {
			fprintf(stderr, "Data (%d bytes) exceeds remaining flash size!\n", endAddr);
			err = -1;
			goto errorOccurred;
		}
		if(pageSize < 128) {
			mask = 127;
		} else {
			mask = pageSize - 1;
		}
		startAddr &= ~mask;                  /* round down */
		endAddr = (endAddr + mask) & ~mask;  /* round up */
		printf("Uploading %d (0x%x) bytes starting at %d (0x%x)\n", endAddr - startAddr, endAddr - startAddr, startAddr, startAddr);
		while(startAddr < endAddr) {
			buffer.data.reportId = 2;
			memcpy(buffer.data.data, dataBuffer + startAddr, 128);
			setUsbInt(buffer.data.address, startAddr, 3);
			printf("\r0x%05x ... 0x%05x", startAddr, startAddr + (int)sizeof(buffer.data.data));
			fflush(stdout);
			if((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, buffer.bytes, sizeof(buffer.data))) != 0) {
				fprintf(stderr, "Error uploading data block: %s\n", usbErrorMessage(err));
				goto errorOccurred;
			}
			startAddr += sizeof(buffer.data.data);
		}
		printf("\n");
		update_dev_info(dev, app_name, app_version);
	}
	if(leaveBootLoader) {
		/* and now leave boot loader: */
		buffer.info.reportId = 1;
		usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, buffer.bytes, sizeof(buffer.info));
		/* Ignore errors here. If the device reboots before we poll the response,
		 * this request fails.
		 */
	}
errorOccurred:
	if(dev != NULL)
		usbCloseDevice(dev);
	return err;
}

/* ------------------------------------------------------------------------- */

static void printUsage(char *pname)
{
	fprintf(stderr, "usage: %s [-r] [-s serial] [-i] [-u app_name app_version] [-f <intel-hexfile-with-flash>]\n", pname);
	fprintf(stderr, "-r\t run the code. If a download is specified - code will be run upon completion.\n");
	fprintf(stderr, "-s\t Use device with this serial number.\n");
	fprintf(stderr, "-i\t Show device info.\n");
	fprintf(stderr, "-l\t List devices.\n");
	fprintf(stderr, "-u\t Set the device info to this, after download\n");


}

int main(int argc, char **argv)
{
	char    *file = NULL;
	char* app_name = NULL;
	char* app_version = NULL;
	char* serial = 0;
	int fcpuonly = 0;

	int i;

	if(argc < 2) {
		printUsage(argv[0]);
		return 1;
	}

	for (i=1; i<argc; i++) {
		if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			printUsage(argv[0]);
			return 1;
		} else if(strcmp(argv[i], "-c") == 0) {
			/* Print out CPU freq only */
			fcpuonly = 1;
		} else if(strcmp(argv[i], "-r") == 0) {
			leaveBootLoader = 1;
		} else if(strcmp(argv[i], "-i") == 0) {
			return dump_device_info(serial, fcpuonly);
		} else if(strcmp(argv[i], "-l") == 0) {
			return list_devices();
		} else if(strcmp(argv[i], "-u") == 0) {
			app_name = argv[i+1];
			app_version = argv[i+2];
		} else if(strcmp(argv[i], "-f") == 0) {
			file = argv[i+1];
		} else if(strcmp(argv[i], "-s") == 0) {
			serial = argv[i+1];
		}
	}

	startAddress = sizeof(dataBuffer);
	endAddress = 0;
	if(file != NULL) {  // an upload file was given, load the data
		if (!app_name) 
			app_name = "hellknowswhat";
		if (!app_version) 
			app_version = "hellknowswhich";
		memset(dataBuffer, -1, sizeof(dataBuffer));
		if(parseIntelHex(file, dataBuffer, &startAddress, &endAddress))
			return 1;
		if(startAddress >= endAddress) {
			fprintf(stderr, "No data in input file, exiting.\n");
			return 1;
		}
	}
		// if no file was given, endAddress is less than startAddress and no data is uploaded
	if(uploadData(dataBuffer, startAddress, endAddress, app_name, app_version, serial))
		return 1;
	return 0;
}

/* ------------------------------------------------------------------------- */


