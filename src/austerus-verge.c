#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
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
	"\n");
}


int main(int argc, char *argv[])
{
	FILE *stream_input;

	int verbose = 0;
	int a = 0;

	const char axes[] = "XY";
	struct limit bounds[4];
	size_t lines = 0;

	// Read command line options
	int option_index = 0, opt=0;
	static struct option loptions[] = {
		{"help", no_argument, 0, 'h'}
	};

	while(opt >= 0) {
		opt = getopt_long(argc, argv, "h", loptions, &option_index);

		switch (opt) {
			case 'h':
				usage();
				return EXIT_SUCCESS;
		}
	}

	stream_input = fopen(argv[optind], "r");

	if (stream_input == NULL) {
		fprintf(stderr, "file error\n");
		return EXIT_FAILURE;
	}
	
	lines = get_extends(bounds, axes, stream_input);

	if (lines == 0) {
		fprintf(stderr, "read no lines\n");
		return EXIT_FAILURE;
	}

	for (a=0; a<strlen(axes); a++)
		printf("%c\t%f\t%f\n", axes[a], bounds[a].min, bounds[a].max);

	fclose(stream_input);

	return EXIT_SUCCESS;
}


