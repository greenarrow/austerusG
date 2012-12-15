#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>

#include "stats.h"


int shift_line(char *line) {

}


int main(int argc, char *argv[]) {
	FILE *stream = stdin;
	FILE *buffer = NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t bytes = 0;

	int mode = NONE;
	char prefix = 0;
	unsigned int code = 0;

	while((bytes = getline(&line, &len, stream) != -1)) {

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
						// shift
						if (mode == RELATIVE)
							fputs(line, stdout);
						else
							fputs(line, stdout); // todo shift
							
						break;
					case 90:
						/* G90 Absolute Positioning */
						mode = ABSOLUTE;
						fputs(line, stdout);
						break;
					case 91:
						/* G91 Relative Positioning */
						fputs(line, stdout);
						mode = RELATIVE;
						break;
					case 92:
						/* G92 Set */
						// shift
						break;
					default:
						// want this? 
						fputs(line, stdout);
				}
				break;

			default:
				fputs(line, stdout);
		}
	}
}
