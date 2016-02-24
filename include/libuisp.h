#ifndef LIBUISP_H
#define LIBUISP_H

#include "usbcalls.h"

#define UISP_PART_NAME_LEN  8

struct partInfo {
	uint16_t      pageSize;
	uint32_t      size;
	uint8_t       ioSize;
	uint8_t       name[UISP_PART_NAME_LEN];
}  __attribute__((packed));

struct deviceInfo {
	uint8_t       reportId;
	uint8_t       numParts;
	uint8_t       cpuFreq;
	struct partInfo parts[];
} __attribute__((packed));



struct deviceInfo *uispReadInfo(usbDevice_t *dev);
usbDevice_t *uispOpen(const char *devId, const char *serial);
char *uispReadPart(usbDevice_t *dev, int part, int *bytes_read);
int uispWritePart(usbDevice_t *dev, int part, const char *buf, int length);
void uispClose(usbDevice_t *dev);
void uispCloseAndRun(usbDevice_t *dev, int part);

void uispPrintInfo(struct deviceInfo *inf);

void uispOverrideLoaderInfo(int vendor, int product, const char *vstring);
void uispProgressCb(void (*cb)(const char *label, int cur, int max));
int uispWritePartFromFile(usbDevice_t *dev, int part, const char *filename);
int uispReadPartToFile(usbDevice_t *dev, int part, const char *filename);

int uispVerifyPart(usbDevice_t *dev, int part, const char *buf, int len);
int uispVerifyPartFromFile(usbDevice_t *dev, int part, const char *filename);
int uispLookupPart(usbDevice_t *dev, const char *name);
#endif
