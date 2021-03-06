#define _POSIX_C_SOURCE /* fmemopen */
#define _GNU_SOURCE /* fmemopen */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "common.h"
#include "gvm.h"


/*
 * Print gcode error "msg" and exit.
 */
static void gcerr(const char *msg)
{
	fprintf(stderr, "gcode error: %s\n", msg);
	exit(1);
}


/*
 * Read a decimal from "string" and return as a long int with 1000 times
 * greater magnitude.
 */
static long int strtoml(char *string)
{
	long int head;
	long int tail;

	char *cursor;

	head = strtol(string, &cursor, 10);

	if (head == LONG_MIN || head == LONG_MAX)
		return head;

	if (cursor == string)
		return head;

	if (strlen(cursor) < 2)
		return head * 1000;

	assert(cursor[0] == '.');

	if (strlen(cursor) > 4)
		cursor[4] = '\0';

	tail = strtol(cursor + 1, &cursor, 10);
	assert(tail < 1000);

	if (tail == LONG_MIN || tail == LONG_MAX)
		return tail;

	if (cursor == string)
		return head;

	while (tail != 0 && tail < 100)
		tail *= 10;


	return (head * 1000) + tail;
}


/*
 * Return true if stream is at the end of the current line.
 */
static bool feol(FILE *stream)
{
	char next;

	next = getc(stream);

	if (next == EOF)
		return true;

	if (next == '\n')
		return true;

	if (ungetc(next, stream) == EOF)
		return true;

	return false;
}


/*
 * Consume and discard bytes from a stream until "stop" is reached.
 */
static void fconsume(FILE *stream, char stop)
{
	char c;

	while (1) {
		c = getc(stream);

		if (c == EOF)
			break;

		if (c == stop)
			break;
	}
}


/*
 * Returns true if "c" indicates remainder of line is valid else returns false.
 * Consumes and discards comments from the stream.
 */
static bool check_line_discard(FILE *stream, char c)
{
	switch (c) {
	case '\n':
		return false;

	case ';':
	case '#':
		fconsume(stream, '\n');
		return false;

	default:
		break;
	}

	return true;
}


/*
 * Initialise virtual machine to default state.
 */
void gvm_init(struct gvm *m, bool verbose)
{
	m->verbose = verbose;
	m->sloppy = true;
	m->unlocated_moves = true;

	m->gcode = NULL;
	m->counter = 0;

	m->mode = MODE_NONE;
	m->located = false;

	point_clear(&(m->position), NULL);
	point_clear(&(m->previous), NULL);
	point_clear(&(m->offset), NULL);
	point_clear(&(m->prevoffset), NULL);
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
 * Decode next line into "cmd" and "result" and set "mask" to denote the axes
 * present in the line.
 */
int gvm_read(struct gvm *m, struct command *cmd, struct point *result,
							enum axismask *mask)
{
	int n;
	char axis;
	char value[128];

	*mask = AXIS_NONE;

	n = fscanf(m->gcode, "%c%u", &(cmd->prefix), &(cmd->code));

	if (n < 1)
		return -1;

	if (n > 0 && !check_line_discard(m->gcode, cmd->prefix))
		return -1;

	if (n < 2 && !m->sloppy)
		gcerr("invalid command");

	if (m->verbose)
		fprintf(stderr, "gvm  [cmd]: %c%d\n", cmd->prefix, cmd->code);

	if (feol(m->gcode))
		return 0;

	while ((n = fscanf(m->gcode, " %c%127[0-9.-]", &axis, value)) == 2) {
		if (m->verbose)
			fprintf(stderr, "gvm [axis]: %c%s\n", axis, value);

		if (!check_line_discard(m->gcode, axis))
			return 0;

		switch (axis) {
		case 'X':
			result->x = strtoml(value);
			*mask |= AXIS_X;

			if (result->x == LONG_MIN || result->x == LONG_MAX)
				gcerr("invalid value");

			break;

		case 'Y':
			result->y = strtoml(value);
			*mask |= AXIS_Y;

			if (result->y == LONG_MIN || result->y == LONG_MAX)
				gcerr("invalid value");

			break;

		case 'Z':
			result->z = strtoml(value);
			*mask |= AXIS_Z;

			if (result->z == LONG_MIN || result->z == LONG_MAX)
				gcerr("invalid value");

			break;

		case 'E':
			result->e = strtoml(value);
			*mask |= AXIS_E;

			if (result->e == LONG_MIN || result->e == LONG_MAX)
				gcerr("invalid value");

			break;

		default:
			break;
		}

		if (feol(m->gcode))
			return 0;
	}

	if (m->sloppy) {
		if (m->verbose) {
			fprintf(stderr, "gvm [warn]: discarding remainder of "
								"line\n");
		}

		check_line_discard(m->gcode, ';');
	} else {
		if (n > 0)
			check_line_discard(m->gcode, axis);
	}

	return 0;
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
			point_delta(&(m->offset), &(m->position), mask, -1);

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
	struct command cmd;
	struct point values;
	enum axismask mask;

	if (!m->gcode) {
		bail("no gcode file has been opened");
		return -1;
	}

	/* record for delta queries */
	m->previous = m->position;
	m->prevoffset = m->offset;

	if (gvm_read(m, &cmd, &values, &mask) == 0)
		gvm_apply(m, &cmd, &values, &mask);

	if (m->verbose) {
		fprintf(stderr, "gvm [resu]: ");
		point_print(stderr, &(m->position));
	}

	if (feof(m->gcode) != 0)
		return -1;

	m->counter++;

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
 * If "physical" is true then use values of real axis positions relative to the
 * homing position.
 */
int gvm_get_position(struct gvm *m, struct point *result, bool physical)
{
	if (!physical) {
		*result = m->position;
		return 0;
	}

	if (!m->located)
		return -1;

	*result = m->position;
	point_delta(result, &(m->offset), NULL, -1);

	return 0;
}


/*
 * Set "result" to the delta incurred by the last step.
 */
int gvm_get_delta(struct gvm *m, struct point *result, bool physical)
{
	point_clear(result, NULL);

	point_delta(result, &(m->position), NULL, 1);
	point_delta(result, &(m->previous), NULL, -1);

	if (physical) {
		if (!m->located)
			return -1;

		point_delta(result, &(m->offset), NULL, -1);
		point_delta(result, &(m->prevoffset), NULL, 1);
	}

	return 0;
}
