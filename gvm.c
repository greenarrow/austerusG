#define _POSIX_C_SOURCE /* fmemopen */
#define _GNU_SOURCE /* fmemopen */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "common.h"
#include "gvm.h"


/*
 * Print gcode error "msg" and exit.
 */
void gcerr(const char *msg)
{
	fprintf(stderr, "gcode error: %s\n", msg);
	exit(1);
}


/*
 * Decode "line" into "cmd" and "result" and set "mask" to denote the axes
 * present in the line.
 */
int decode_line(char *line, struct command *cmd, struct point *result,
							enum axismask *mask)
{
	FILE *stream;

	char axis;
	float value;

	*mask = AXIS_NONE;

	if (strlen(line) == 0)
		return -1;

	/* comment */
	if (line[0] == ';')
		return -1;

	if (line[0] == '#')
		return -1;

	stream = fmemopen(line, strlen(line), "r");

	if (stream == NULL)
		bail("decode_line");

	if (fscanf(stream, " %c%u", &(cmd->prefix), &(cmd->code)) != 2)
		gcerr("invalid command");

	while (fscanf(stream, " %c%f", &axis, &value) == 2) {
		switch (axis) {
		case 'X':
			result->x = value;
			*mask |= AXIS_X;
			break;

		case 'Y':
			result->y = value;
			*mask |= AXIS_Y;
			break;

		case 'Z':
			result->z = value;
			*mask |= AXIS_Z;
			break;

		case 'E':
			result->e = value;
			*mask |= AXIS_E;
			break;

		case ';':
		case '#':
			/* comment */
			goto finish;

		default:
			break;
		}
	}

finish:
	if (fclose(stream) != 0)
		bail("decode_line");

	return 0;
}


/*
 * Initialise virtual machine to default state.
 */
void gvm_init(struct gvm *m, bool verbose)
{
	m->verbose = verbose;
	m->unlocated_moves = false;

	m->gcode = NULL;
	m->counter = 0;

	m->mode = MODE_NONE;
	m->located = false;

	point_clear(&(m->position), NULL);
	point_clear(&(m->previous), NULL);
	point_clear(&(m->offset), NULL);
}


/*
 * Load gcode file "path".
 */
void gvm_load(struct gvm *m, const char *path)
{
	if (m->gcode)
		bail("gcode file already open");

	if (m->verbose)
		fprintf(stderr, "gvm [load]: %s\n", path);

	m->gcode = fopen(path, "r");

	if (m->gcode == NULL)
		bail("gvm_load");
}


/*
 * Close open gcode file.
 */
void gvm_close(struct gvm *m)
{
	if (fclose(m->gcode) != 0)
		bail("gvm_close");

	m->gcode = NULL;
}


/*
 * Apply a command to the virtual machine.:
 */
void gvm_apply(struct gvm *m, struct command *cmd, struct point *values,
							enum axismask *mask)
{
	switch (cmd->prefix) {
	case 'G':
		switch (cmd->code) {
		case 0:
		case 1:
			/* G0 / G1 Move */
			if (!m->unlocated_moves && !m->located)
				gcerr("un-located moves not enabled");

			switch (m->mode) {
			case MODE_ABSOLUTE:
				point_cpy(&(m->position), values, mask);
				break;

			case MODE_RELATIVE:
				point_delta(&(m->position), values, mask, 1);
				break;

			default:
				gcerr("mode undeclared");
			}
			break;

		case 28:
			/* G28 Home */
			point_clear(&(m->position), mask);
			point_clear(&(m->offset), mask);

			m->located = true;
			break;

		case 90:
			/* G90 Absolute Positioning */
			m->mode = MODE_ABSOLUTE;
			break;

		case 91:
			/* G91 Relative Positioning */
			m->mode = MODE_RELATIVE;
			break;

		case 92:
			/* G92 Set */
			point_delta(&(m->offset), values, mask, 1);
			point_delta(&(m->offset), &(m->position), NULL, -1);

			point_cpy(&(m->position), values, mask);
			break;
		}
		break;

	case ';':
	case '#':
		/* comment */
		break;
	}
}


/*
 * Step through the next line of gcode in the open file.
 */
int gvm_step(struct gvm *m)
{
	char *line = NULL;
	size_t len = 0;

	struct command cmd;
	struct point values;
	enum axismask mask;

	if (!m->gcode)
		bail("no gcode file has been opened");

	if (getline(&line, &len, m->gcode) == -1) {
		if (line)
			free(line);

		return -1;
	}

	if (m->verbose)
		fprintf(stderr, "gvm [line]: %s", line);

	/* record for delta queries */
	m->previous = m->position;

	if (decode_line(line, &cmd, &values, &mask) == 0)
		gvm_apply(m, &cmd, &values, &mask);

	m->counter++;

	if (line)
		free(line);

	return 0;
}


/*
 * Run entire gcode file.
 */
void gvm_run(struct gvm *m)
{
	while (gvm_step(m) != -1);
}


/*
 * Return gcode line counter.
 */
unsigned int gvm_get_counter(struct gvm *m)
{
	return m->counter;
}


/*
 * Set "result" to values of current axis positions.
 */
int gvm_get_position(struct gvm *m, struct point *result)
{
	*result = m->position;
	return 0;
}


/*
 * Set "result" to values of real axis positions relative to the homing
 * position.
 */
int gvm_get_physical(struct gvm *m, struct point *result)
{
	if (!m->located)
		return -1;

	*result = m->position;
	point_delta(result, &(m->offset), NULL, -1);

	return 0;
}


/*
 * Set "result" to the delta incurred by the last step.
 */
void gvm_get_delta(struct gvm *m, struct point *result)
{
	point_clear(result, NULL);

	point_delta(result, &(m->position), NULL, 1);
	point_delta(result, &(m->previous), NULL, -1);
}
