#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>

#include "stats.h"
#include "austerus-verge.h"


// Print usage to terminal
void usage(void) {
	printf("Usage: austerus-verge [OPTION]... [FILE]\n"
	"\n"
	"Options:\n"
	" -h, --help             Print this help message\n"
	" -d, --deposition       Bounds of deposited material\n"
	" -p, --physical         Track physical location not axis values\n"
	" -v, --verbose          Explain what is being done\n"
	"\n");
}


int main(int argc, char *argv[])
{
	FILE *stream_input;

	int a = 0;

	char axes[5];
	struct limit bounds[4];
	size_t lines = 0;

	bool deposition = false;
	bool physical = false;

	bool verbose = false;

	// Read command line options
	int option_index = 0, opt=0;
	static struct option loptions[] = {
		{"help", no_argument, 0, 'h'},
		{"deposition", no_argument, 0, 'd'},
		{"physical", no_argument, 0, 'p'},
		{"verbose", no_argument, 0, 'v'}
	};

	while(opt >= 0) {
		opt = getopt_long(argc, argv, "hdpv", loptions, &option_index);

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
			case 'v':
				verbose = true;
				break;
		}
	}

	if (deposition)
		 strcpy(axes, "XYZE");
	else
		 strcpy(axes, "XYZ");

	stream_input = fopen(argv[optind], "r");

	if (stream_input == NULL) {
		fprintf(stderr, "file error\n");
		return EXIT_FAILURE;
	}
	
	lines = get_extends(bounds, axes, deposition, physical, verbose,
		stream_input);

	if (lines == 0) {
		fprintf(stderr, "read no lines\n");
		return EXIT_FAILURE;
	}

	for (a=0; a<strlen(axes); a++)
		printf("%c\t%f\t%f\n", axes[a], bounds[a].min, bounds[a].max);

	fclose(stream_input);

	return EXIT_SUCCESS;
}


