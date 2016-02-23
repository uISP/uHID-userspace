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
#include <stdint.h>
#include <errno.h>
#include <libuisp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


static int   IDENT_VENDOR_NUM          =    0x1d50;
static int   IDENT_PRODUCT_NUM         =    0x6032;
static char  IDENT_VENDOR_STRING[255]  =    "uHID";

#define min_t(type, a, b) (((type)(a)<(type)(b))?(type)(a):(type)(b))

static void (*progresscb)(const char *label, int cur, int max);


void uispProgressCb(void (*cb)(const char *label, int cur, int max))
{
	progresscb = cb;
}

static void show_progress(const char *label, int cur, int max)
{
	if (progresscb)
		progresscb(label, cur, max);
}

/** 
 * Override usb device information
 * 
 * @param vendor 
 * @param product 
 * @param vstring 
 */
void uispOverrideLoaderInfo(int vendor, int product, int vstring)
{
	IDENT_VENDOR_NUM = vendor;
	IDENT_PRODUCT_NUM = product;
	strncpy(IDENT_VENDOR_STRING, vstring, 255);
	IDENT_VENDOR_STRING[254] = 0x0;
}

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

static char    *usbErrorMessage(int errCode)
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


/** 
 * Reads the information struct from the device. The caller must free the 
 * struct obtained. 
 * 
 * @param dev 
 * 
 * @return 
 */
struct deviceInfo *uispReadInfo(usbDevice_t *dev)
{
	int len = 255;
	int i; 
	struct deviceInfo *inf = calloc(len, 1);
	if (!inf)
		goto error;

	if ((usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 0, (char *) inf, &len)) != 0) {
		goto error;
	}

	/* Sanity checking and force strings end with zeroes just in case */

	
	if ((len < sizeof(struct deviceInfo)) || 
	    (len < sizeof(struct deviceInfo) + inf->numParts * sizeof(struct partInfo))) {
		fprintf(stderr, "Short-read on deviceInfo - bad bootloader version?\n");
		fprintf(stderr, "Expected %lu bytes, got %d bytes (%lu + %d * %lu)\n", 
			sizeof(struct deviceInfo) + inf->numParts * sizeof(struct partInfo),
			len, 
			sizeof(struct deviceInfo), 2, sizeof(struct partInfo));
		exit(1);
	}

	for (i=0; i<inf->numParts; i++) { 
		inf->parts[i].name[UISP_PART_NAME_LEN -1] = 0;
	}

	return inf;
error:
	if (inf)
		free(inf);
	return NULL;
			
}


/** 
 * Open a uisp device. This function will pick the device with USB serial number
 * 
 * @param serial USB Serial Number string to match
 * 
 * @return 
 */
usbDevice_t *uispOpen(const char *devId, const char *serial)
{
	usbDevice_t *dev = NULL;
	int err;
	if((err = usbOpenDevice(&dev, 
				IDENT_VENDOR_NUM, IDENT_VENDOR_STRING, 
				IDENT_PRODUCT_NUM, devId, 
				serial)) != 0) {
		fprintf(stderr, "Error opening uHID device: %s\n", usbErrorMessage(err));
		return NULL;
	}
	return dev;
}


/** 
 * Read a partition to a character buffer. Returns a pointer to the allocated buffer or NULL.
 * If the buffer 
 * 
 * @param dev 
 * @param part 
 * @param bytes_read 
 * 
 * @return 
 */
char *uispReadPart(usbDevice_t *dev, int part, int *bytes_read)
{
	struct deviceInfo *inf = uispReadInfo(dev);
	if (!inf)
		return NULL;
	if (part <= 0)
		return NULL;
	if (part > inf->numParts) 
		return NULL;
	
	uint32_t size = inf->parts[part-1].size;
	char *tmp = malloc(size);
	if (!tmp)
		goto errfreeinf;

	int pos = 0;
	while (pos < size) { 
		int len = min_t(int, size - pos, inf->parts[part-1].ioSize);
		if ((usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 
				  part, (char *) &tmp[pos], &len)) != 0) {
			goto errfreetmp;
		}
		pos+=len;
		show_progress("Reading", pos, size);
	}

	if (bytes_read)
		*bytes_read = pos; 
	free(inf);
	show_progress("Reading", size, size);
	return tmp;
errfreetmp:
	free(tmp);
errfreeinf:
	free(inf);
	return NULL;
		
}

/** 
 * Write data from buffer to partition
 * 
 * @param dev 
 * @param part 
 * @param buf 
 * @param length 
 * 
 * @return 
 */
int uispWritePart(usbDevice_t *dev, int part, const char *buf, int length)
{
	int ret=0;
	struct deviceInfo *inf = uispReadInfo(dev);
	if (!inf)
		return -ENOENT;
	if (part <= 0)
		return -ENOENT;
	if (part > inf->numParts) 
		return -ENOENT;
	
	uint32_t size = inf->parts[part-1].size;
	int pos = 0;
	while (pos < size) { 
		int len = min_t(int, size - pos, inf->parts[part-1].ioSize);
		if ((usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, part,
				  (char *) &buf[pos], len)) != 0) {
			ret = -EIO;
			break;
		}
		pos+=len;
		show_progress("Writing", pos, size);
	}
	free(inf);
	show_progress("Writing", size, size);
	return ret;		
}

int uispLookupPart(usbDevice_t *dev, const char *name)
{
	struct deviceInfo *inf = uispReadInfo(dev);
	if (!inf)
		return -1;
	int ret = -1;
	int i;
	for (i=0; i < inf->numParts; i++) {
		if (strcmp(name, inf->parts[i].name)==0) {
			ret = i + 1;
			break;
		}
	}
	free(inf);
	return ret; 
}

int uispVerifyPart(usbDevice_t *dev, int part, const char *buf, int len)
{
	int bytes;
	void *pbuf = uispReadPart(dev, part, &bytes);
	if (buf == NULL)
		return -1;
	return memcmp(buf, pbuf, len);
}

int uispVerifyPartFromFile(usbDevice_t *dev, int part, const char *filename)
{
	int len; 

	char *pbuf = uispReadPart(dev, part, &len);
	if (!pbuf)
		return -1;

	int ret; 
	int len_file;
	struct stat st;

	ret = stat(filename, &st);

	if (ret != 0)
		return ret; 

	len_file = (int ) st.st_size;
	if (len_file < len)
		len = len_file; 

	char *buf = malloc(len); 
	if (!buf) { 
		ret = -ENOMEM;
		goto errfreepbuf;
	}

	FILE *fd = fopen(filename, "r");	
	if (!fd) { 
		ret = -EIO;
		goto errfreebuf;
	}
	
	ret = fread(buf, len, 1, fd);
	if (ret != 1) {
		ret = -EIO;
		goto errclose;
	}
	
	ret = uispVerifyPart(dev, part, buf, len);

errclose:
	fclose(fd);
errfreebuf:
	free(buf);
errfreepbuf:
	free(pbuf);
	return ret;
}

int uispReadPartToFile(usbDevice_t *dev, int part, const char *filename)
{
	int bytes;
	void *buf = uispReadPart(dev, part, &bytes);
	if (buf == NULL)
		return -1;

	FILE *fd = fopen(filename, "w+");
	if (!fd)
		return -errno;
	int ret = fwrite(buf, bytes, 1, fd); 
	fclose(fd);
	return (ret == 0);
}


int uispWritePartFromFile(usbDevice_t *dev, int part, const char *filename)
{
	struct deviceInfo *inf = uispReadInfo(dev);
	if (!inf)
		return -1;

	if (part > inf->numParts)
		return -1;

	int ret; 
	int len = inf->parts[part-1].size; 
	int len_file;
	struct stat st;

	ret = stat(filename, &st);

	if (ret != 0)
		return ret; 

	len_file = (int ) st.st_size;
	if (len_file < len)
		len = len_file; 

	char *buf = malloc(len); 
	if (!buf) { 
		ret = -ENOMEM;
		goto errfreeinf;
	}

	FILE *fd = fopen(filename, "r");	
	if (!fd) { 
		ret = -EIO;
		goto errfreebuf;
	}
	
	ret = fread(buf, len, 1, fd);
	if (ret != 1) {
		ret = -EIO;
		goto errclose;
	}
	
	ret = uispWritePart(dev, part, buf, len);

errclose:
	fclose(fd);
errfreebuf:
	free(buf);
errfreeinf:
	free(inf);
	return ret;
}

void uispClose(usbDevice_t *dev)
{
	usbCloseDevice(dev);	
}

void uispCloseAndRun(usbDevice_t *dev, int part)
{	
	char tmp[8];	
	usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 0,
		     tmp, 1);
	uispClose(dev);
}


void uispPrintInfo(struct deviceInfo *inf)
{
	int i;

	printf("Partitions:        %d\n", inf->numParts);
	printf("CPU Frequency:     %.1f Mhz\n", inf->cpuFreq / 10.0);
	for (i=0; i<inf->numParts; i++) { 
		struct partInfo *p = &inf->parts[i];
		printf("%d. %s %d bytes (%d byte pages, %d bytes per packet)  \n", 
		       i, p->name, p->size, p->pageSize, p->ioSize);		
	}
}

