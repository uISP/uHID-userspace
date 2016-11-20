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

	*dev = uispOpen(NULL, NULL);
	if (!*dev)
		bailout(1);
	if (tmp)
		free(tmp);
}

static void usage(const char *name)
{
	printf("uHID bootloader tool (c) Andrew 'Necromant' Andrianov 2016\n"
	       "This is free software subject to GPLv2 license.\n\n"
	       "Usage: \n"
	       "%s --part eeprom --write file.bin\n"
	       "%s --part eeprom --read  file.bin\n"
	       "", name, name);
}

int main(int argc, char **argv)
{
	int ret = 0;
	hid_device *uisp = NULL;
	int part;
	struct deviceInfo *inf;

	const char *product = NULL;
	const char *serial = NULL;
	const char *filename;
	uispProgressCb(progressbar);

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
			check_and_open(&uisp, product, serial);
			inf = uispReadInfo(uisp);
			uispPrintInfo(inf);
			free(inf);
			bailout(0);
			break;
		case 'r':
			filename = optarg;
			check_and_open(&uisp, product, serial);
			part = uispLookupPart(uisp, partname);
			if (part < 0) {
				fprintf(stderr, "No such part");
				bailout(1);
			}
			printf("Reading partition %d (%s) from %s\n", part, partname, filename);
			bailout(uispReadPartToFile(uisp, part, filename));
			break;
		case 'w':
			filename = optarg;
			check_and_open(&uisp, product, serial);
			part = uispLookupPart(uisp, partname);
			if (part < 0) {
				fprintf(stderr, "No such part");
				bailout(1);
			}
			printf("Writing partition %d (%s) from %s\n", part, partname, filename);
			ret = uispWritePartFromFile(uisp, part, filename);
			printf("\n");
			if (ret)
				bailout(ret);

			if (!verify)
				break;
		case 'v':
			filename = optarg;
			check_and_open(&uisp, product, serial);
			part = uispLookupPart(uisp, partname);
			if (part < 0) {
				fprintf(stderr, "No such part");
				bailout(1);
			}
			printf("Verifying partition %d (%s) from %s\n", part, partname, filename);
			ret = uispVerifyPartFromFile(uisp, part, filename);
			if (ret ==0 )
				printf("\n Verification completed without error\n");
			else
				printf("\n Something bad during verification \n");
			bailout(ret);
		case 'R':
			check_and_open(&uisp, product, serial);
			uispCloseAndRun(uisp, part);
			bailout(0);
			break;
		}
	}
	bailout(0);
}
