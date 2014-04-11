#include <stdio.h>

#include "common.h"
#include "point.h"


/*
 * Zero values in point using mask if not NULL.
 */
void point_clear(struct point *value, enum axismask *mask)
{
	if (!mask || *mask & AXIS_X)
		value->x = 0;

	if (!mask || *mask & AXIS_Y)
		value->y = 0;

	if (!mask || *mask & AXIS_Z)
		value->z = 0;

	if (!mask || *mask & AXIS_E)
		value->e = 0;
}


/*
 * Copy values from src to dest using mask if not NULL.
 */
void point_cpy(struct point *dst, struct point *src, enum axismask *mask)
{
	if (!mask || *mask & AXIS_X)
		dst->x = src->x;

	if (!mask || *mask & AXIS_Y)
		dst->y = src->y;

	if (!mask || *mask & AXIS_Z)
		dst->z = src->z;

	if (!mask || *mask & AXIS_E)
		dst->e = src->e;
}


/*
 *
 * Apply delta to value in given direction using mask if not NULL.
 */
void point_delta(struct point *value, struct point *delta, enum axismask *mask,
								int direction)
{
	if (direction > 0) {
		if (!mask || *mask & AXIS_X)
			value->x += delta->x;

		if (!mask || *mask & AXIS_Y)
			value->y += delta->y;

		if (!mask || *mask & AXIS_Z)
			value->z += delta->z;

		if (!mask || *mask & AXIS_E)
			value->e += delta->e;
	} else {
		if (!mask || *mask & AXIS_X)
			value->x -= delta->x;

		if (!mask || *mask & AXIS_Y)
			value->y -= delta->y;

		if (!mask || *mask & AXIS_Z)
			value->z -= delta->z;

		if (!mask || *mask & AXIS_E)
			value->e -= delta->e;
	}
}


/*
 * Debugging function to print stuct pos.
 */
void point_print(FILE *stream, struct point *value)
{
	fprintf(stream, "X%f Y%f Z%f E%f\n", (float)value->x / 1000.0,
			(float)value->y / 1000.0, (float)value->z / 1000.0,
			(float)value->e / 1000.0);
}
