#ifndef LIBUISP_H
#define LIBUISP_H

#define UISP_PART_NAME_LEN  8
#include <hidapi/hidapi.h>

struct partInfo {
	uint16_t      pageSize;
	uint32_t      size;
	uint8_t       ioSize;
	uint8_t       name[UISP_PART_NAME_LEN];
}  __attribute__((packed));

struct deviceInfo {
	uint8_t       reportId;
	uint8_t       reportId2; /*FixMe: This should be ditched from fw */
	uint8_t       numParts;
	uint8_t       cpuFreq;
	struct partInfo parts[];
} __attribute__((packed));



struct deviceInfo *uispReadInfo(hid_device *dev);
hid_device *uispOpen(const wchar_t *devId, const wchar_t *serial);
char *uispReadPart(hid_device *dev, int part, int *bytes_read);
int uispWritePart(hid_device *dev, int part, const char *buf, int length);
void uispClose(hid_device *dev);
void uispCloseAndRun(hid_device *dev, int part);

void uispPrintInfo(struct deviceInfo *inf);

void uispOverrideLoaderInfo(int vendor, int product, const wchar_t *vstring);
void uispProgressCb(void (*cb)(const char *label, int cur, int max));
int uispWritePartFromFile(hid_device *dev, int part, const char *filename);
int uispReadPartToFile(hid_device *dev, int part, const char *filename);

int uispVerifyPart(hid_device *dev, int part, const char *buf, int len);
int uispVerifyPartFromFile(hid_device *dev, int part, const char *filename);
int uispLookupPart(hid_device *dev, const char *name);
#endif
