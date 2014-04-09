#define _GNU_SOURCE /* getline */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>

#include "gvm.h"


/*
 * Check to see if the target axis is specified in the line.
 */
bool check_axis(const char *line, char target)
{
	char *found = strchr(line, target);
	return found != NULL;
}


/*
 * Check to see if any of the target axes are specified in the line.
 */
bool check_axes(const char *line, char *targets)
{
	int i = 0;

	for (i = 0; i < strlen(targets); i++) {
		if (check_axis(line, targets[i]))
			return true;
	}

	return false;
}


void usage(void)
{
	printf("Usage: austerus-shift [OPTION]...\n"
	"\n"
	"Options:\n"
	" -h               Print this help message\n"
	" -x DELTA         Shift X axis by DELTA\n"
	" -y DELTA         Shift Y axis by DELTA\n"
	" -a X             Safe X start\n"
	" -b Y             Safe Y start\n"
	" -z SAFE          Safe to move anywhere at Z SAFE\n"
	"\n");
}


int main(int argc, char *argv[]) {
	FILE *stream = stdin;
	FILE *buffer = NULL;

	int opt = 0;

	char *line = NULL;
	size_t len = 0;
	ssize_t bytes = 0;

	enum coordmode mode = MODE_NONE;
	char prefix = 0;
	unsigned int code = 0;

	float dx = 0.0;
	float dy = 0.0;
	float zmin = 0.0;
	float sx = 0.0;
	float sy = 0.0;

	bool init = true;
	bool drifting = true;

	while((opt = getopt(argc, argv, "hx:y:z:a:b:")) != -1) {
		switch (opt) {
		case 'h':
			usage();
			return EXIT_SUCCESS;
		case 'x':
			dx = strtof(optarg, NULL);
			break;
		case 'y':
			dy = strtof(optarg, NULL);
			break;
		case 'z':
			zmin = strtof(optarg, NULL);
			break;
		case 'a':
			sx = strtof(optarg, NULL);
			break;
		case 'b':
			sy = strtof(optarg, NULL);
			break;
		}
	}

	/* need more stict exit cases */

	while ((bytes = getline(&line, &len, stream) != -1)) {
		buffer = fmemopen(line, strlen(line), "r");

		if (fscanf(buffer, "%c%u", &prefix, &code) != 2) {
			fputs(line, stdout);
			continue;
		}

		switch (prefix) {
		case 'G':
			switch (code) {
			case 1:
				/* G1 Move */
				if (mode == MODE_RELATIVE)
					fputs(line, stdout);
				else
					fputs(line, stdout);

				if (check_axes(line, "XY"))
					init = false;

				break;
			case 28:
				/* G28 Home */
				if (check_axes(line, "XY")) {
					printf("G91\n");
					printf("G1 Z%f\n", zmin);
					printf("G90\n");

					printf("G28 X0 Y0\n");

					/* breaking */
					if (drifting) {
						printf("G28 Z0\n");
						printf("G1 Z%f\n", zmin);
					}

					if (dx < 0.0) {
						printf("G92 X%f\n", dx);
						printf("G1 X%f\n", sx - dx);
					} else if (dx > 0.0) {
						printf("G1 X%f\n", sx);
						printf("G92 X%f\n", sx - dx);
					}

					if (dy < 0.0) {
						printf("G92 Y%f\n", sy);
						printf("G1 Y%f\n", sy - dy);
					} else if (dy > 0.0) {
						printf("G1 Y%f\n", sy);
						printf("G92 Y%f\n", sy - dy);
					}

					drifting = false;
				}

				break;
			case 90:
				/* G90 Absolute Positioning */
				mode = MODE_ABSOLUTE;
				fputs(line, stdout);
				break;
			case 91:
				/* G91 Relative Positioning */
				fputs(line, stdout);
				mode = MODE_RELATIVE;
				break;
			case 92:
				/* G92 Set */
				if (check_axes(line, "XYZ")) {
					if (init)
						break;

					fputs("G92 not allowed here\n", stderr);
					exit(1);
				}

				fputs(line, stdout);
				break;
			default:
				fputs(line, stdout);
			}
			break;

		default:
			fputs(line, stdout);
		}
	}

	return 0;
}
