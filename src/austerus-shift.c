#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>

#include "stats.h"


int shift_line(char *line) {

}

/*
 * To use actual shifting of value or just place move and set commands?
 *  when to do this?
 *  ideally want no extra physical movements? Well we may have to!
 * Test by comparing verges
 * Should be eager to bail if gcode not well understood
 * Ultimately we need more than just shifting, we need to avoid all objects on
 *  the bed with all motions!
 * 1. Simple single z clearance option
 * 2. Do our own thing ignoring gcode to get safely to start of safe zone for
 *    printing.
 *    - homeing, moving, setting (the safety dance)
 * 3. Process remaining gcode squashing first home, first move to print area?
 *
 * In fact, can we just replace any homing occurance with our own?
 * but we also want to get rid of the existing print relative to home offset.
 *
 * Cut off any moves before 'start'? (bring in stats functions?)
 *   do we need functions to find starting line, and rewind file?
 *
 * The detection of the point of actual print starting is key
 * Then we just need to make sure we get there and have created a suitable offset?
 *
 *
 * Plan
 *
 * + Need to draw diagram of shifted object to get idea!
 *
 * 1. Write a simple stats function that returns line of deposition start.
 * 2. Use this function folllowed by a simple rewind.
 * 3. 
 *
 * The result should be:
 * 1. Home X, Y
 * 2. Home Z (planner should check this)
 * 3. Allow lines that do not move axes
 * 4. Move Z to safe height
 * 5. Perform offset move and reset position to X0 Y0
 *    (no this will not work if offset is negative
 * 5. Perform move to dep start X and Y
 * 6. Perform offset move
 * 7. Reset position to dep start
 * 8. Perform move to dep start Z
 * 9. Allow all gcode
 * 10. Bail if there is any more homing.
 * 11. Squash any weird moves?
 *
 */


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
					case 28:
						/* G28 Home */
						
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
