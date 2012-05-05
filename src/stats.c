#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stats.h"


/*
 * Read a stream for the next occurrence of a value for the specified axis.
 */
int read_axis(FILE *stream, char target, float *value)
{
	char axis = 0;
	float previous = *value;

	while (fscanf(stream, " %c%f", &axis, value) == 2) {
		if (axis == target)
			return 1;
	}

	*value = previous;
	return 0;
}


/*
 * Read delta E and new E position from G1 move line.
 */
void read_move(FILE *buffer, int mode, float *delta, float *position)
{
	float previous = *position;

	switch (mode) {
		case ABSOLUTE:
			read_axis(buffer, 'E', position);
			*delta = *position - previous;
			break;
		case RELATIVE:
			read_axis(buffer, 'E', delta);
			*position += *delta;
			break;
		default:
			fprintf(stderr, "read_move: Positioning mode not set");
			abort();
	}
}


/*
 * Calculate the new E and delta E for a gcode line.
 */
int read_extruded_delta(char *line, int *mode, float *delta, float *position)
{
	char prefix = 0;
	unsigned int code = 0;

	float previous = 0.0;

	FILE *buffer = fmemopen(line, strlen(line), "r");

	if (fscanf(buffer, "%c%u", &prefix, &code) != 2)
		return 0;

	switch (prefix) {
		case 'G':
			switch (code) {
				case 1:
					/* G1 Move */
					read_move(buffer, *mode, delta,
							position);
				case 90:
					/* G90 Absolute Positioning */
					*mode = ABSOLUTE;
					break;
				case 91:
					/* G91 Relative Positioning */
					*mode = RELATIVE;
					break;
				case 92:
					/* G92 Set */
					read_axis(buffer, 'E', position);
					*delta = 0.0;
					break;
			}
			break;

		case ';':
		case '#':
			/* Comment */
			return 0;
	}

	fclose(buffer);
	return 1;
}


/*
 * Generate an array containing the total length of filament extruded at the
 * end of each line in the gcode file.
 */
float get_progress_table(unsigned int **table, size_t *lines, FILE *stream)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t bytes = 0;

	static const size_t grow = 2000;
	size_t capacity = grow;

	float position = 0.0;
	float extruded = 0.0;
	float delta = 0.0;

	int mode = NONE;

	if (*table == NULL) {
		*table = (unsigned int*)
			malloc(capacity * sizeof(unsigned int));
		}

	*lines = 0;

	while((bytes = getline(&line, &len, stream) != -1))
	{
		if (bytes == 0)
			continue;

		if (read_extruded_delta(line, &mode, &delta, &position) == 0)
			continue;

		extruded += delta;
		(*table)[*lines] = (unsigned int) extruded;
		(*lines)++;

		/* Grow the table if necessary */
		if (*lines >= capacity) {
			capacity += grow;
			*table = realloc(*table, capacity *
						sizeof(unsigned int));
		}
	}

	/* Free any un-used memory */
	if (*lines < capacity)
		*table = realloc(*table, *lines * sizeof(unsigned int));

	free(line);

	return extruded;
}

