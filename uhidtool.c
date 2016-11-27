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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <libuhid.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#endif
#include <getopt.h>


static  int verify = 1;
static 	const char *partname;
enum {
	OP_NONE = 0,
	OP_READ,
	OP_WRITE,
	OP_VFY
};

static struct option long_options[] =
{
	/* These options set a flag. */
	{"help",     no_argument,       0, 'h'},
	{"part",     required_argument, 0, 'p'},
	{"write",    required_argument, 0, 'w'},
	{"read",     required_argument, 0, 'r'},
	{"verify",   required_argument, 0, 'v'},
	{"product",  required_argument, 0, 'P'},
	{"serial",   required_argument, 0, 'S'},
	{"info",     no_argument,       0, 'i'},
	{"run",      no_argument,       0, 'R'},
	{0, 0, 0, 0}
};

void progressplain(const char *label, int value, int max)
{
	printf("%s %d/%d\n", label, value, max);
}
void progressbar(const char *label, int value, int max)
{
	value = max - value;
	float percent = 100.0 - (float) value * 100.0 / (float) max;
	int cols;

#ifndef _WIN32
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	cols = w.ws_col;
#else
	cols = 80;
#endif

	int txt = printf("%s %.02f %% done [", label, percent);
	int max_bars = cols - txt - 7;
	int bars = max_bars - (int)(((float) value * max_bars) / (float) max);

	if (max_bars > 0) {
		int i;
		for (i=0; i<bars; i++)
			printf("#");
		for (i=bars; i<max_bars; i++)
			printf(" ");
		printf("]\r");
	}
	if (value == max)
		printf("\n");
	fflush(stdout);

}

static void bailout(int code)
{
/* A little feature for windoze junkies */
#ifdef _WIN32
	printf("Press any key to exit...\n");
	getchar();
#endif
	exit(code);
}

static void check_and_open(hid_device **dev, const char *product, const char *serial)
{
	if (*dev)
		return;

	wchar_t *tmp = NULL;

	if (serial) {
	  tmp = malloc(strlen(serial) * (sizeof(wchar_t) + 1));
		if (!tmp) {
			fprintf(stderr, "Out of memory\n");
			bailout(1);
		}
		mbstowcs(tmp, serial, strlen(serial));
	}

	*dev = uhidOpen(NULL);
	if (!*dev)
		bailout(1);
	if (tmp)
		free(tmp);
}

static void usage(const char *name)
{
	/* Avoid showing long paths in windoze */
	const char *nm = strrchr(name, '\\');
	if (!nm)
		nm = name;
	else
		nm++;

	printf("uHID bootloader tool (c) Andrew 'Necromant' Andrianov 2016\n"
	       "This is free software subject to GPLv2 license.\n\n"
	       "Usage: \n"
	       "%s --help                      - This help message\n"
				 "%s --info                      - Show info about device\n"
	       "%s --part eeprom --write 1.bin - Write partition eeprom with 1.bin\n"
	       "%s --part eeprom --read  1.bin - Read partition eeprom to 1.bin\n"
				 "%s --run [flash]               - Execute code in partition [flash]\n"
				 "                                 Optional, if supported by target MCU\n"
				 "\n"
				 "uHIDtool can read intel hex as well as binary. \n"
				 "The filename extension should be .ihx or .hex for it to work\n"
	       "", nm, nm, nm, nm, nm);
}

int main(int argc, char **argv)
{
	int ret = 0;
	hid_device *uhid = NULL;
	int part;
	struct uHidDeviceInfo *inf;

	const char *product = NULL;
	const char *serial = NULL;
	const char *filename;
	uhidProgressCb(progressbar);

	if (argc == 1) {
		usage(argv[0]);
		bailout(1);
	}

	while (1) {
		int option_index = 0;
		int c;
		c = getopt_long (argc, argv, "hp:w:r:p:v:P:S:",
				 long_options, &option_index);
		if (c == -1)
			break;
		switch(c) {
		case 'h':
			usage(argv[0]);
			bailout(1);
			break;
		case 'p':
			partname = optarg;
			break;
		case 'P':
			product = optarg;
			break;
		case 'i':
			check_and_open(&uhid, product, serial);
			inf = uhidReadInfo(uhid);
			uhidPrintInfo(inf);
			free(inf);
			bailout(0);
			break;
		case 'r':
			filename = optarg;
			check_and_open(&uhid, product, serial);
			part = uhidLookupPart(uhid, partname);
			if (part < 0) {
				fprintf(stderr, "No such part");
				bailout(1);
			}
			printf("Reading partition %d (%s) from %s\n", part, partname, filename);
			bailout(uhidReadPartToFile(uhid, part, filename));
			break;
		case 'w':
			filename = optarg;
			check_and_open(&uhid, product, serial);
			part = uhidLookupPart(uhid, partname);
			if (part < 0) {
				fprintf(stderr, "No such part");
				bailout(1);
			}
			printf("Writing partition %d (%s) from %s\n", part, partname, filename);
			ret = uhidWritePartFromFile(uhid, part, filename);
			printf("\n");
			if (ret)
				bailout(ret);

			if (!verify)
				break;
		case 'v':
			filename = optarg;
			check_and_open(&uhid, product, serial);
			part = uhidLookupPart(uhid, partname);
			if (part < 0) {
				fprintf(stderr, "No such part");
				bailout(1);
			}
			printf("Verifying partition %d (%s) from %s\n", part, partname, filename);
			ret = uhidVerifyPartFromFile(uhid, part, filename);
			if (ret ==0 )
				printf("\n Verification completed without error\n");
			else
				printf("\n Something bad during verification \n");
			bailout(ret);
		case 'R':
			check_and_open(&uhid, product, serial);
			uhidCloseAndRun(uhid, part);
			bailout(0);
			break;
		}
	}
	bailout(0);
}
