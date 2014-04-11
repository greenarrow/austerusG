#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>

#include "stats.h"


static float read_part(char *arg)
{
	char *part = strtok(arg, ":");

	if (part == NULL) {
		fprintf(stderr, "invalid ignore string\n");
		abort();
	}

	return strtof(part, NULL);
}


/*
 * Print usage to terminal
 */
static void usage(void)
{
	printf("Usage: austerus-verge [OPTION]... [FILE]\n"
	"\n"
	"Options:\n"
	" -h, --help             Print this help message\n"
	" -d, --deposition       Bounds of deposited material\n"
	" -p, --physical         Track physical location not axis values\n"
	" -z, --zmin=zmin        Track bounds travelled while Z less than\n"
	" -v, --verbose          Explain what is being done\n"
	"\n");
}


int main(int argc, char *argv[])
{
	size_t lines = 0;

	struct extends bounds;

	bool deposition = false;
	bool physical = false;
	bool zmode = false;
	unsigned long zmin = 0;
	struct region head;
	struct region *ignore = NULL;

	bool verbose = false;

	int option_index = 0, opt=0;
	static struct option loptions[] = {
		{"help", no_argument, 0, 'h'},
		{"deposition", no_argument, 0, 'd'},
		{"physical", no_argument, 0, 'p'},
		{"zmin", required_argument, 0, 'z'},
		{"ignore", required_argument, 0, 'i'},
		{"verbose", no_argument, 0, 'v'}
	};

	while(opt >= 0) {
		opt = getopt_long(argc, argv, "hdpz:i:v", loptions, &option_index);

		switch (opt) {
			case 'h':
				usage();
				return EXIT_SUCCESS;
			case 'd':
				deposition = true;
				break;
			case 'p':
				physical = true;
				break;
			case 'z':
				zmode = true;
				zmin = (long int)(atof(optarg) * 1000.0);
				break;
			case 'i':
				head.x1 = read_part(optarg);
				head.x2 = read_part(NULL);
				head.y1 = read_part(NULL);
				head.y2 = read_part(NULL);

				ignore = &head;
				break;
			case 'v':
				verbose = true;
				break;
		}
	}

	if (deposition && zmode) {
		fprintf(stderr, "deposition and zmin modes cannot be used "
			"together\n");
		return EXIT_FAILURE;
	}

	bounds_clear(&bounds);

	lines = get_extends(&bounds, deposition, physical, zmode, zmin,
		ignore, verbose, argv[optind]);

	if (lines == 0) {
		fprintf(stderr, "read no lines\n");
		return EXIT_FAILURE;
	}

	printf("X\t%f\t%f\n", (float)bounds.x.min / 1000.0,
						(float)bounds.x.max / 1000.0);
	printf("Y\t%f\t%f\n", (float)bounds.y.min / 1000.0,
						(float)bounds.y.max / 1000.0);
	printf("Z\t%f\t%f\n", (float)bounds.z.min / 1000.0,
						(float)bounds.z.max / 1000.0);

	if (deposition)
		printf("E\t%f\t%f\n", (float)bounds.e.min / 1000.0,
						(float)bounds.e.max / 1000.0);

	return EXIT_SUCCESS;
}
