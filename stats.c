#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include "point.h"
#include "gvm.h"
#include "stats.h"

#define MIN(p, q) (((p) < (q)) ? (p) : (q))
#define MAX(p, q) (((p) >= (q)) ? (p) : (q))


void bounds_clear(struct extends *value)
{
	value->x.min = LONG_MAX;
	value->x.max = LONG_MIN;

	value->y.min = LONG_MAX;
	value->y.max = LONG_MIN;

	value->z.min = LONG_MAX;
	value->z.max = LONG_MIN;

	value->e.min = LONG_MAX;
	value->e.max = LONG_MIN;
}


/*
 * Generate an array containing the total length of filament extruded at the
 * end of each line in the gcode file.
 */
float get_progress_table(unsigned int **table, size_t *lines,
							const char *filename)
{
	struct gvm m;
	struct point delta;

	static const size_t grow = 2000;
	size_t capacity = grow;

	float extruded = 0.0;

	if (*table == NULL) {
		*table = (unsigned int*)malloc(capacity *
							sizeof(unsigned int));
	}

	*lines = 0;

	gvm_init(&m, false);
	gvm_load(&m, filename);

	while (gvm_step(&m) != -1) {
		gvm_get_delta(&m, &delta, true);
		extruded += delta.e;

		(*table)[*lines] = (unsigned int)extruded;
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

	gvm_close(&m);

	return extruded;
}


/*
 * Calculate the extends reached while printing gcode data.
 * When deposition=false this includes all movements.
 * When deposition=true this includes only movements that deposit material on
 * the print bed.
 */
size_t get_extends(struct extends *bounds, bool deposition,
	bool physical, bool zmode, long int zmin, struct region *ignore,
	bool verbose, const char *filename)
{
	bool started = false;
	bool igthis = false;
	bool iglast = false;

	struct gvm m;

	struct point pos;
	struct point delta;

	if (deposition && zmode) {
		fprintf(stderr, "deposition and zmode cannot be used\n");
		abort();
	}

	gvm_init(&m, verbose);
	gvm_load(&m, filename);

	while (gvm_step(&m) != -1) {
		/* in physical mode skip updating stats if machine is not
		 * physically located */
		if (gvm_get_position(&m, &pos, physical) == -1)
			continue;

		if (gvm_get_delta(&m, &delta, physical) == -1)
			continue;

		/* Always update E bounds. */
		bounds->e.min = MIN(bounds->e.min, pos.e);
		bounds->e.max = MAX(bounds->e.max, pos.e);

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

		bounds->x.min = MIN(bounds->x.min, pos.x);
		bounds->x.max = MAX(bounds->x.max, pos.x);

		bounds->y.min = MIN(bounds->y.min, pos.y);
		bounds->y.max = MAX(bounds->y.max, pos.y);

		bounds->z.min = MIN(bounds->z.min, pos.z);
		bounds->z.max = MAX(bounds->z.max, pos.z);

		if (!iglast) {
			bounds->x.min = MIN(bounds->x.min, pos.x - delta.x);
			bounds->x.max = MAX(bounds->x.max, pos.x - delta.x);

			bounds->y.min = MIN(bounds->y.min, pos.y - delta.y);
			bounds->y.max = MAX(bounds->y.max, pos.y - delta.y);

			bounds->z.min = MIN(bounds->z.min, pos.z - delta.z);
			bounds->z.max = MAX(bounds->z.max, pos.z - delta.z);
		}

		iglast = igthis;
	}

	gvm_close(&m);

	return gvm_get_counter(&m);
}
