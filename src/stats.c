#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "stats.h"


/*
 * Return the position of axis in the axes string or -1 if no contained.
 */
int axis_position(const char *axes, char axis) {
	char *found = strchr(axes, axis);

	if (found == NULL)
		return -1;

	return found - axes;
}


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
void read_move(FILE *buffer, int mode, char axis, float *delta, float *position)
{
	float previous = *position;

	switch (mode) {
		case ABSOLUTE:
			read_axis(buffer, axis, position);
			*delta = *position - previous;
			break;
		case RELATIVE:
			read_axis(buffer, axis, delta);
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
int read_axis_delta(char *line, const char axis, int *mode, float *delta,
	float *position)
{
	char prefix = 0;
	unsigned int code = 0;

	FILE *buffer = fmemopen(line, strlen(line), "r");

	if (fscanf(buffer, "%c%u", &prefix, &code) != 2)
		return 0;

	switch (prefix) {
		case 'G':
			switch (code) {
				case 1:
					/* G1 Move */
					read_move(buffer, *mode, axis, delta,
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
					read_axis(buffer, axis, position);
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

		if (read_axis_delta(line, 'E', &mode, &delta, &position) == 0)
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


/*
 * Calculate the extends reached while printing gcode data.
 * When deposition=false this includes all movements.
 * When deposition=true this includes only movements that deposit material on
 * the print bed.
 */
size_t get_extends(struct limit *bounds, const char *axes, bool deposition,
	FILE *stream)
{
	char *line = NULL;
	size_t lines = 0;
	size_t len = 0;
	ssize_t bytes = 0;

	int mode = NONE;

	int ix = axis_position(axes, 'X');
	int iy = axis_position(axes, 'Y');
	int iz = axis_position(axes, 'Z');
	int ie = axis_position(axes, 'E');

	if (deposition && ie == -1) {
		fprintf(stderr, "axes XY are required in deposition mode\n");
		abort();
	}

	float delta[strlen(axes)];
	float position[strlen(axes)];

	int a = 0;

	bool started = false;

	for (a=0; a<strlen(axes); a++) {
		position[a] = 0.0;
		delta[a] = 0.0;
		bounds[a].min = FLT_MAX;
		bounds[a].max = FLT_MIN;
	}

	while((bytes = getline(&line, &len, stream) != -1))
	{
		if (bytes == 0)
			continue;

		for (a=0; a<strlen(axes); a++) {
			if (read_axis_delta(line, axes[a], &mode, &delta[a],
				&position[a]) != 1)
				continue;
			/*
			 * In deposition mode only record extends while
			 * depositing
			 */
			if (deposition && (!started || delta[ie] == 0.0))
				continue;

			bounds[a].min = fminf(bounds[a].min, position[a]);
			bounds[a].max = fmaxf(bounds[a].max, position[a]);
		}

		if (deposition && !started) {
			/*
			 * When print head is moved while extruding for first
			 * time consider to have started printing.
			 */
			if (delta[ie] > 0.0 && delta[ix] + delta[iy] > 0.0) {
				started = true;

				for (a=0; a<strlen(axes); a++) {
					bounds[a].min = fminf(bounds[a].min,
						position[a] - delta[a]);
					bounds[a].max = fmaxf(bounds[a].max,
						position[a] - delta[a]);
				}
			}
		}
		lines++;
	}

	free(line);

	return lines;
}
