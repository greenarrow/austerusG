#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "point.h"
#include "gvm.h"
#include "stats.h"


void bounds_clear(struct extends *value)
{
	value->x.min = FLT_MAX;
	value->x.max = FLT_MIN;

	value->y.min = FLT_MAX;
	value->y.max = FLT_MIN;

	value->z.min = FLT_MAX;
	value->z.max = FLT_MIN;

	value->e.min = FLT_MAX;
	value->e.max = FLT_MIN;
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

	int mode = MODE_NONE;

	if (*table == NULL) {
		*table = (unsigned int*)
			malloc(capacity * sizeof(unsigned int));
		}

	*lines = 0;

	while((bytes = getline(&line, &len, stream) != -1))
	{
		if (bytes == 0)
			continue;

/*
		if (read_axis_delta(line, 'E', &mode, &delta, &position,
			&offset) == 0)
			continue;
*/

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
size_t get_extends(struct extends *bounds, bool deposition,
	bool physical, bool zmode, float zmin, struct region *ignore,
	bool verbose, const char *filename)
{
	bool started = false;
	bool igthis = false;
	bool iglast = false;

	struct gvm m;

	struct point pos;
	struct point delta;

	int rc;

	if (deposition && zmode) {
		fprintf(stderr, "deposition and zmode cannot be used\n");
		abort();
	}

	gvm_init(&m, verbose);
	gvm_load(&m, filename);

	while (gvm_step(&m) != -1) {
		if (physical)
			rc = gvm_get_physical(&m, &pos);
		else
			rc = gvm_get_position(&m, &pos);

		/* continue if machine is not physically located */
		if (rc == -1)
			continue;

		gvm_get_delta(&m, &delta);

		/* Always update E bounds. */
		bounds->e.min = fminf(bounds->e.min, pos.e);
		bounds->e.max = fmaxf(bounds->e.max, pos.e);

		igthis = false;

		/* See if new point is inside ignore region. */
		if (ignore != NULL) {
			if (pos.x >= ignore->x1 && pos.y <= ignore->x2
					&& pos.y >= ignore->y1
					&& pos.y <= ignore->y2) {

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
			if (delta.e > 0.0 && delta.x + delta.y > 0.0) {
				started = true;

				if (verbose)
					fprintf(stderr, "DEPOSITION STARTED\n");
			}
		}

		/*
		 * In deposition mode only record extends while
		 * depositing.
		 */
		if (deposition && (!started || delta.e <= 0.0))
			continue;

		/*
		 * In zmode only record extends while Z axis is within
		 * unsafe area.
		 */
		if (zmode && pos.z > zmin && pos.z - delta.z > zmin)
			continue;

		bounds->x.min = fminf(bounds->x.min, pos.x);
		bounds->x.max = fmaxf(bounds->x.max, pos.x);

		bounds->y.min = fminf(bounds->y.min, pos.y);
		bounds->y.max = fmaxf(bounds->y.max, pos.y);

		bounds->z.min = fminf(bounds->z.min, pos.z);
		bounds->z.max = fmaxf(bounds->z.max, pos.z);

		if (!iglast) {
			bounds->x.min = fminf(bounds->x.min, pos.x - delta.x);
			bounds->x.max = fmaxf(bounds->x.max, pos.x - delta.x);

			bounds->y.min = fminf(bounds->y.min, pos.y - delta.y);
			bounds->y.max = fmaxf(bounds->y.max, pos.y - delta.y);

			bounds->z.min = fminf(bounds->z.min, pos.z - delta.z);
			bounds->z.max = fmaxf(bounds->z.max, pos.z - delta.z);
		}

		iglast = igthis;
	}

	gvm_close(&m);

	return gvm_get_counter(&m);
}
