/*
 * patch_qboot.c
 *
 *  Created on: May 21, 2013
 *      Author: drivers
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <getopt.h>     /* for getopt() */
#include <stdarg.h>
#include <string.h>

#define SEARCH_SPACE	(16 * 1024)
#define CMDLINE_MAX			256
#define MACHID_MAX			16
#define ATAG_ADDR_MAX		16
#define KERNEL_ADDR_MAX		16

static char* progname;
static char* input_file;

static char* cmdline;
static char* mach_id;
static char* atag_addr;
static char* kernel_addr;

static void usage(int status)
{
	FILE *stream = (status != EXIT_SUCCESS) ? stderr : stdout;

	fprintf(stream, "Usage: %s <image file> [OPTIONS...]\n", progname);
	fprintf(stream,
"\n"
"Options:\n"
"  -c <cmdline>       command line string\n"
"  -m <mach id>       machine type\n"
"  -a <atag addr>     atag address\n"
"  -k <kernel addr>   kernel address\n"
"  -h                 show this screen\n"
"\n"
"Example:\n"
"%s qboot.bin -c \"console=ttyS1,115200 init=/linuxrc mem=32M\" -m 3300 -a 0x46000100 -k 0x46008000\n",
	progname);

	exit(status);
}

int main(int argc, char **argv)
{
	int fd, found = 0, len, ret = -1;
	char *ptr, *p;

	progname = (char*)basename(argv[0]);

	if (argc < 2)
		usage(EXIT_FAILURE);

	input_file = argv[1];

	while (1) {
		int c;

		c = getopt(argc, argv, "c:m:a:k:h");
		if (c == -1)
			break;

		switch (c) {
		case 'c':
			cmdline = optarg;
			break;
		case 'm':
			mach_id = optarg;
			break;
		case 'a':
			atag_addr = optarg;
			break;
		case 'k':
			kernel_addr = optarg;
			break;
		case 'h':
			usage(EXIT_SUCCESS);
			break;
		default:
			usage(EXIT_FAILURE);
			break;

		}
	}

	if (cmdline != NULL) {
		len = strlen(cmdline);
		if (len > CMDLINE_MAX) {
			fprintf(stderr, "Command line string too long\n");
			goto err1;
		}
	}

	if (((fd = open(input_file, O_RDWR)) < 0) ||
		(ptr = (char *) mmap(0, SEARCH_SPACE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == (void *) (-1)) {
		fprintf(stderr, "Could not open input image");
		goto err2;
	}

	/* patch command line */
	if (cmdline != NULL) {
		found = 0;
		for (p = ptr; p < (ptr + SEARCH_SPACE); p += 4) {
			if (memcmp(p, "CMDLINE:", 8) == 0) {
				found = 1;
				p += 8;
				break;
			}
		}

		if (found) {
			memset(p, 0, CMDLINE_MAX);
			strcpy(p, cmdline);
			msync(p, CMDLINE_MAX, MS_SYNC | MS_INVALIDATE);

		} else {
			fprintf(stderr, "Command line marker not found!\n");
		}
	}

	/* patch machine ID */
	if (mach_id != NULL) {
		found = 0;
		for (p = ptr; p < (ptr + SEARCH_SPACE); p += 4) {
			if (memcmp(p, "MACHID:", 7) == 0) {
				found = 1;
				p += 7;
				break;
			}
		}

		if (!found) {
			fprintf(stderr, "Machine ID marker not found!\n");

		} else {

			memset(p, 0, MACHID_MAX);
			strcpy(p, mach_id);
			msync(p, MACHID_MAX, MS_SYNC | MS_INVALIDATE);
		}
	}


	/* patch ATAG address */
	if (atag_addr != NULL) {
		found = 0;
		for (p = ptr; p < (ptr + SEARCH_SPACE); p += 4) {
			if (memcmp(p, "ATAGADDR:", 9) == 0) {
				found = 1;
				p += 9;
				break;
			}
		}

		if (!found) {
			fprintf(stderr, "ATAG Address marker not found!\n");
		} else {

			memset(p, 0, ATAG_ADDR_MAX);
			strcpy(p, atag_addr);
			msync(p, ATAG_ADDR_MAX, MS_SYNC | MS_INVALIDATE);
		}
	}

	/* patch machine ID */
	if (kernel_addr != NULL) {
		found = 0;
		for (p = ptr; p < (ptr + SEARCH_SPACE); p += 4) {
			if (memcmp(p, "KERNELADDR:", 11) == 0) {
				found = 1;
				p += 11;
				break;
			}
		}

		if (!found) {
			fprintf(stderr, "Kernel Address marker not found!\n");

		} else {
			memset(p, 0, KERNEL_ADDR_MAX);
			strcpy(p, kernel_addr);
			msync(p, KERNEL_ADDR_MAX, MS_SYNC | MS_INVALIDATE);
		}
	}

	munmap((void *) ptr, SEARCH_SPACE);
err2:
	if (fd > 0)
		close(fd);
err1:
	return ret;
}
