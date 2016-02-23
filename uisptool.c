#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <libuisp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <getopt.h>


static  int verify = 1;
static  int op;
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

void progressbar(char *label, int value, int max)
{
	value = max - value;
	float percent = 100.0 - (float) value * 100.0 / (float) max;
	int cols;
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	cols = w.ws_col;

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
 
static void check_and_open(usbDevice_t **dev, char *product, char *serial)
{
	if (*dev)
		return;
	*dev = uispOpen(product, serial);
	if (!*dev)
		exit(1);
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
	int err;
	int ret = 0;

	usbDevice_t *uisp = NULL;
	int op; 
	int part; 

	const char *product = NULL;
	const char *serial = NULL;
	const char *filename; 
	uispProgressCb(progressbar);

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
			break;
		case 'p':
			partname = optarg;
			break;
		case 'P':
			product = optarg;
			break;
		case 'i':
			check_and_open(&uisp, product, serial);
			uispPrintInfo(uisp);
			break;
		case 'r':
			filename = optarg;
			check_and_open(&uisp, product, serial);
			part = uispLookupPart(uisp, partname);
			if (part < 0) {
				fprintf(stderr, "No such part");
				exit(1);
			}
			printf("Reading partition %d (%s) from %s\n", part, partname, filename);
			return uispReadPartToFile(uisp, part, filename);
			break;
		case 'w':
			filename = optarg;
			check_and_open(&uisp, product, serial);
			part = uispLookupPart(uisp, partname);
			if (part < 0) {
				fprintf(stderr, "No such part");
				exit(1);
			}
			printf("Writing partition %d (%s) from %s\n", part, partname, filename);
			ret = uispWritePartFromFile(uisp, part, filename);
			printf("\n");
			if (ret)
				return ret;

			if (!verify)
				break;
		case 'v':
			filename = optarg;
			check_and_open(&uisp, product, serial);
			part = uispLookupPart(uisp, partname);
			if (part < 0) {
				fprintf(stderr, "No such part");
				exit(1);
			}
			printf("Verifying partition %d (%s) from %s\n", part, partname, filename);
			ret = uispVerifyPartFromFile(uisp, part, filename);
			if (ret ==0 )
				printf("\n Verification completed without error\n");
			else
				printf("\n Something bad during verification \n");
			return ret;
		case 'R':
			check_and_open(&uisp, product, serial);
			uispCloseAndRun(uisp, part);
			return 0;
			break;
		}
	}
	return 0;
}
