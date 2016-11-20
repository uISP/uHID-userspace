/*
 *  uHID Universal MCU Bootloader. Library.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <hidapi/hidapi.h>
#include <libuhid.h>


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
		{ /* Sentinel */ }
};

#define min_t(type, a, b) (((type)(a)<(type)(b))?(type)(a):(type)(b))

static void (*progresscb)(const char *label, int cur, int max);


void UHID_API uhidProgressCb(void (*cb)(const char *label, int cur, int max))
{
	progresscb = cb;
}

static void show_progress(const char *label, int cur, int max)
{
	if (progresscb)
		progresscb(label, cur, max);
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

static ssize_t  parseIntelHex(const char *hexfile, char *buffer, int *startAddr, int *endAddr)
{
	ssize_t     address, base, d, segment, i, lineLen, sum;
	ssize_t maxAddress = 0;

	FILE    *input;

	input = fopen(hexfile, "r");
	if(input == NULL) {
		fprintf(stderr, "error opening %s: %s\n", hexfile, strerror(errno));
		return -1;
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
			if (buffer)
				buffer[address] = d;
			address++;
			if (address > maxAddress)
				maxAddress = address;
			sum += d;
		}
		sum += parseHex(input, 2);
		if((sum & 0xff) != 0) {
			fprintf(stderr, "Warning: Checksum error between address 0x%zx and 0x%zx\n", base, address);
		}
		if(*startAddr > base)
			*startAddr = base;
		if(*endAddr < address)
			*endAddr = address;
	}
	fclose(input);
	return maxAddress;
}


/**
 * Reads the information struct from the device. The caller must free the
 * struct obtained.
 *
 * @param dev
 *
 * @return
 */
UHID_API struct uHidDeviceInfo *uhidReadInfo(hid_device *dev)
{
	int len = 255;
	int i;
	struct uHidDeviceInfo *inf = calloc(len, 1);
	if (!inf)
		goto error;

	len = hid_get_feature_report(dev, (unsigned char *)inf, 255);
	if (len < 0) {
		fprintf(stderr, "Error reading info struct: %ls\n", hid_error(dev));
		goto error;
	}

	/* Sanity checking and force strings end with zeroes just in case */
	if ((len < sizeof(struct uHidDeviceInfo)) ||
	    (len < sizeof(struct uHidDeviceInfo) + inf->numParts * sizeof(struct uHidPartInfo))) {
		fprintf(stderr, "Short-read on uHidDeviceInfo - bad bootloader version?\n");
		fprintf(stderr, "Expected %lu bytes, got %d bytes (%lu + %d * %lu)\n",
			sizeof(struct uHidDeviceInfo) + inf->numParts * sizeof(struct uHidPartInfo),
			len,
			sizeof(struct uHidDeviceInfo), 2, sizeof(struct uHidPartInfo));
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


static int hidDevMatch(struct hid_device_info *inf,
											struct uHidDeviceMatch *deviceMatch)
{
	if (inf->vendor_id != deviceMatch->vendor)
		return 0;

	if (inf->product_id != deviceMatch->product)
		return 0;

	if (deviceMatch->vendorName &&
			(wcscmp(inf->manufacturer_string, deviceMatch->vendorName)!=0))
			return 0;

	if (deviceMatch->productName &&
			(wcscmp(inf->product_string, deviceMatch->productName)!=0))
			return 0;

	if (deviceMatch->productName &&
  		(wcscmp(inf->serial_number, deviceMatch->serialNumber)!=0))
			return 0;

	return 1;
}


/**
 * Returns a linked list of compatible uHID devices found on the system
 *
 * If deviceMatch is NULL uHID will search for devices based on a built-in table
 * of compatible devices
 *
 * @param deviceMatch Should be an array of suitable matches terminated
 * by an all-zero element.
 *
 * @return device instance or NULL on error
 */
UHID_API struct hid_device_info *uhidListDevices(struct uHidDeviceMatch *deviceMatch)
{
	/* TODO: Implement */
	return NULL;
}

/**
 * Open a uHID device. if @deviceMatch table is supplied uHid will find
 * a device described there.
 *
 * if  is NULL uHID will search for devices based on a built-in table
 *
 * @param deviceMatch Should be an array of suitable matches terminated
 * by an all-zero element.
 *
 * @return device instance or NULL on error
 */
UHID_API hid_device *uhidOpen(struct uHidDeviceMatch *deviceMatch)
{
	hid_device *dev = NULL;

	if (!deviceMatch)
		deviceMatch = compatibleDevices;

	struct hid_device_info *inf_list = hid_enumerate(0, 0);
	struct hid_device_info *inf = inf_list;
	struct hid_device_info *found = NULL;

	while (inf->next) {
		struct uHidDeviceMatch *tmp = deviceMatch;
		while (tmp->vendor) {
			if (hidDevMatch(inf, tmp)) {
				found = inf;
				break;
			}

			if (found)
				break;

			tmp++;
		}
		inf = inf->next;
	};

	if (!found)
		goto bailout;

	dev = hid_open_path(found->path);
	if (!dev)
		fprintf(stderr, "Failed to open a uHID device (Permissions problem?)\n");

bailout:
	hid_free_enumeration(inf_list);
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
UHID_API char *uhidReadPart(hid_device *dev, int part, int *bytes_read)
{
	struct uHidDeviceInfo *inf = uhidReadInfo(dev);
	if (!inf)
		return NULL;
	if (part <= 0)
		return NULL;
	if (part > inf->numParts)
		return NULL;

	uint32_t size = inf->parts[part-1].size;
	unsigned char *tmp = malloc(size);
	if (!tmp)
		goto errfreeinf;

	int pos = 0;
	while (pos < size) {
		int len = min_t(int, size - pos, inf->parts[part-1].ioSize);
		tmp[pos] = part;
		len = hid_get_feature_report(dev, &tmp[pos], len);
		if (len < 0)
					goto errfreetmp;

		pos+=len;
		show_progress("Reading", pos, size);
	}

	if (bytes_read)
		*bytes_read = pos;
	free(inf);
	show_progress("Reading", size, size);
	return (char *) tmp;
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
UHID_API int uhidWritePart(hid_device *dev, int part, const char *buf, int length)
{
	int ret=0;
	struct uHidDeviceInfo *inf = uhidReadInfo(dev);
	if (!inf)
		return -ENOENT;
	if (part <= 0)
		return -ENOENT;
	if (part > inf->numParts)
		return -ENOENT;

	int pageSize = inf->parts[part-1].pageSize;
	uint32_t size = inf->parts[part-1].size;
	if (length > size) {
		printf("WARNING: Input file buffer exceeds the target partition size\n");
		printf("WARNING: The data will be truncated\n");
	}

	size = min_t(uint32_t, size, length);

	if (size % pageSize)
		size += pageSize - (size % pageSize);

	char *destbuf = calloc(1, inf->parts[part-1].ioSize + 1);
	destbuf[0]=part;

	int pos = 0;
	while (pos < size) {
		int len = min_t(int, size - pos, inf->parts[part-1].ioSize);
		memcpy(&destbuf[1], &buf[pos], len);
		len = hid_send_feature_report(dev, (unsigned char*) destbuf, len);
		if (len < 0) {
			ret = -EIO;
			break;
		}

		pos+=len;
		show_progress("Writing", pos, size);
	}

	free(inf);
	free(destbuf);
	show_progress("Writing", size, size);
	return ret;
}

UHID_API int uhidLookupPart(hid_device *dev, const char *name)
{
	struct uHidDeviceInfo *inf = uhidReadInfo(dev);
	if (!inf)
		return -1;
	int ret = -1;
	int i;
	for (i=0; i < inf->numParts; i++) {
		if (strcmp(name, (char *) inf->parts[i].name)==0) {
			ret = i + 1;
			break;
		}
	}
	free(inf);
	return ret;
}

UHID_API int uhidVerifyPart(hid_device *dev, int part, const char *buf, int len)
{
	int bytes;
	void *pbuf = uhidReadPart(dev, part, &bytes);

	if (pbuf == NULL)
		return -1;

	bytes = min_t(int, bytes, len);
	return memcmp(buf, pbuf, bytes);
}


UHID_API int uhidReadPartToFile(hid_device *dev, int part, const char *filename)
{
	int bytes;
	void *buf = uhidReadPart(dev, part, &bytes);
	if (buf == NULL)
		return -1;

	FILE *fd = fopen(filename, "w+");
	if (!fd)
		return -errno;
	int ret = fwrite(buf, bytes, 1, fd);
	fclose(fd);
	return (ret == 0);
}


static int guessIfIntelHex(const char *filename)
{
	char *ext = strrchr(filename, '.');
  /* Assume bin by default */
	if (!ext)
	   return 0;
  if (strcmp(ext,".ihx")==0)
    return 1;
  if (strcmp(ext,".hex")==0)
    return 1;
  return 0;
}

static ssize_t getFileContents(const char* filename, char **buf)
{
  int ok;
  FILE *fd = fopen(filename, "r");
  ssize_t sz;
	if (fd == NULL)
		return -1;

	fseek(fd, 0, SEEK_END);
	sz = ftell(fd);
  rewind(fd);

  *buf = malloc(sz);
  if (!*buf)
    return -1;

  ok=fread(*buf, sz, 1, fd);
	fclose(fd);

  if (ok)
    return sz;
  else {
    free(*buf);
    return -1;
  }
}

UHID_API int uhidWritePartFromFile(hid_device *dev, int part, const char *filename)
{
        struct uHidDeviceInfo *inf = uhidReadInfo(dev);
        if (!inf)
                return -1;

        if (part > inf->numParts)
                return -1;

        int ret = -EIO;
        ssize_t len_file;
        ssize_t len = inf->parts[part-1].size;
        char *buf;

        if (!guessIfIntelHex(filename))
        {
                len_file = getFileContents(filename, &buf);
                if (len_file <=0)
                  goto errfreeinf;
                printf("Input file detected as binary\n");
        } else {
              int startAddr, endAddr;
              len_file = parseIntelHex(filename, NULL, &startAddr, &endAddr);
              if (len_file <= 0)
                  goto errfreeinf;
              printf("Input file detected as Intel Hex\n");
              printf("Start addr 0x%x end addr 0x%x\n",
                startAddr, endAddr);
              buf = malloc(len_file);
              if (!buf)
                goto errfreeinf;
              len_file = parseIntelHex(filename, buf, &startAddr, &endAddr);
              if (len_file <=0)
                  goto errfreebuf;
        }

        printf("Length: %zd bytes\n", len_file);

        if (len_file < len)
                len = len_file;

        ret = uhidWritePart(dev, part, buf, len);

errfreebuf:
        free(buf);
errfreeinf:
        free(inf);
        return ret;
}

UHID_API int uhidVerifyPartFromFile(hid_device *dev, int part, const char *filename)
{

	ssize_t len_file;
  char *buf;
  len_file = getFileContents(filename, &buf);
  if (len_file <= 0)
    return -1;

	return uhidVerifyPart(dev, part, buf, len_file);

}


UHID_API void uhidClose(hid_device *dev)
{
	hid_close(dev);
}

UHID_API void uhidCloseAndRun(hid_device *dev, int part)
{
	char tmp[8];
	tmp[0]=0x0;
	tmp[1]=part;
	hid_send_feature_report(dev, (unsigned char *) tmp, part);
	uhidClose(dev);
}


UHID_API void uhidPrintInfo(struct uHidDeviceInfo *inf)
{
	int i;

	printf("Partitions:        %d\n", inf->numParts);
	printf("CPU Frequency:     %.1f Mhz\n", inf->cpuFreq / 10.0);
	for (i=0; i<inf->numParts; i++) {
		struct uHidPartInfo *p = &inf->parts[i];
		printf("%d. %s %d bytes (%d byte pages, %d bytes per packet)  \n",
		       i, p->name, p->size, p->pageSize, p->ioSize);
	}
}
