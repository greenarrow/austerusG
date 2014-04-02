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


/*
 * Read the value of the target axis from the line.
 */
bool read_axis(const char *line, char target, float *value)
{
	char axis = 0;
	char *found = strchr(line, target);

	if (found == NULL)
		return false;
	
	if (sscanf(found, "%c%f", &axis, value) != 2) {
		fprintf(stderr, "read_axis: Failed to read axis %c\n", target);
		abort();
	}

	return true;
}


/*
 * Read delta E and new E position from G1 move line.
 */
void read_move(const char *line, int mode, char axis, float *delta,
	float *position)
{
	float previous = *position;

	switch (mode) {
		case ABSOLUTE:
			read_axis(line, axis, position);
			*delta = *position - previous;
			break;
		case RELATIVE:
			*delta = 0.0;
			read_axis(line, axis, delta);
			*position += *delta;
			break;
		default:
			fprintf(stderr, "read_move: Positioning mode not set\n");
			abort();
	}
}


/*
 * Calculate the new E and delta E for a gcode line.
 */
int read_axis_delta(const char *line, const char axis, int *mode, float *delta,
	float *position, float *offset)
{
	char prefix = 0;
	unsigned int code = 0;

	float ignore = 0.0;

	if (sscanf(line, "%c%u", &prefix, &code) != 2)
		return 0;

	switch (prefix) {
		case 'G':
			switch (code) {
				case 0:
				case 1:
					/* G0 / G1 Move */
					read_move(line, *mode, axis, delta,
						position);
					break;
				case 28:
					/* G28 Home */
					if (read_axis(line, axis, &ignore)) {
						*position = 0.0;
						*offset = 0.0;
					}
					break;
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
					*offset -= *position;
					read_axis(line, axis, position);

					*offset += *position;
					*delta = 0.0;
					break;
			}
			break;

		case ';':
		case '#':
			/* Comment */
			return 0;
	}

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
	float offset = 0.0;

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

		if (read_axis_delta(line, 'E', &mode, &delta, &position,
			&offset) == 0)
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
	bool physical, bool zmode, float zmin, struct region *ignore,
	bool verbose, FILE *stream)
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

	if (deposition && zmode) {
		fprintf(stderr, "deposition and zmode cannot be used\n");
		abort();
	}

	if (deposition && ie == -1) {
		fprintf(stderr, "axes XY are required in deposition mode\n");
		abort();
	}

	float delta[strlen(axes)];
	float position[strlen(axes)];
	float offset[strlen(axes)];

	int a = 0;

	bool started = false;

	bool igthis = false;
	bool iglast = false;

	for (a=0; a<strlen(axes); a++) {
		position[a] = 0.0;
		delta[a] = 0.0;
		offset[a] = 0.0;
		bounds[a].min = FLT_MAX;
		bounds[a].max = FLT_MIN;
	}

	while((bytes = getline(&line, &len, stream) != -1)) {
		if (verbose)
			fprintf(stderr, "LINE: \"%s\"\n", line);

		if (bytes == 0)
			continue;

		for (a=0; a<strlen(axes); a++) {
			read_axis_delta(line, axes[a], &mode, &delta[a],
						&position[a], &offset[a]);
		}

		/* Always update E bounds. */
		bounds[ie].min = fminf(bounds[ie].min,
			position[ie] - (physical ? offset[ie]: 0.0));
		bounds[ie].max = fmaxf(bounds[ie].max,
			position[ie] - (physical ? offset[ie] : 0.0));

		if (verbose) {
			fprintf(stderr, "STATE: deposition %d, "
				"started %d, delta[E] %f\n",
				deposition, started, delta[ie]);

			fprintf(stderr, "START CONSIDER: delta[E] %f, "
				"delta[X] %f, delta[Y] %f\n", delta[ie],
				delta[ix], delta[iy]);
		}

		igthis = false;

		/* See if new point is inside ignore region. */
		if (ignore != NULL) {
			if ((position[ix] - (physical ? offset[ix] : 0.0)) >= ignore->x1
				&& (position[ix] - (physical ? offset[ix] : 0.0)) <= ignore->x2
				&& (position[iy] - (physical ? offset[iy] : 0.0)) >= ignore->y1
				&& (position[iy] - (physical ? offset[iy] : 0.0)) <= ignore->y2) {

				if (verbose)
					fprintf(stderr, "ignore region\n");

				igthis = true;
				iglast = true;
				continue;
			}
		}

		/*
		 * When print head is moved while extruding for first
		 * time consider to have started printing.
		 */
		if (deposition && !started) {
			if (delta[ie] > 0.0 && delta[ix] + delta[iy] > 0.0) {
				started = true;

				if (verbose)
					fprintf(stderr, "DEPOSITION STARTED\n");
			}
		}

		/*
		 * In deposition mode only record extends while
		 * depositing.
		 */
		if (deposition && (!started || delta[ie] <= 0.0))
			continue;

		/*
		 * In zmode only record extends while Z axis is within
		 * unsafe area.
		 */
		if (zmode && position[iz] > zmin && position[iz] - delta[iz] > zmin)
			continue;

		for (a=0; a<strlen(axes); a++) {
			if (axes[a] == 'E')
				continue;

			bounds[a].min = fminf(bounds[a].min,
				position[a] - (physical ? offset[a]: 0.0));
			bounds[a].max = fmaxf(bounds[a].max,
				position[a] - (physical ? offset[a] : 0.0));

			if (!iglast) {
				bounds[a].min = fminf(bounds[a].min,
					position[a] - delta[a] - (physical ? offset[a]: 0.0));
				bounds[a].max = fmaxf(bounds[a].max,
					position[a] - delta[a] - (physical ? offset[a] : 0.0));
			}

			if (verbose)
				fprintf(stderr, "bound[%c].min = %f, "
						"max = %f\n", axes[a],
						bounds[a].min, bounds[a].max);
		}

		iglast = igthis;
		lines++;
	}

	free(line);

	return lines;
}
