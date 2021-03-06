#define _BSD_SOURCE /* usleep */
#define _GNU_SOURCE /* getline */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "serial.h"
#include "protocol.h"
#include "austerus-core.h"
#include "defaults.h"


/* User options. Global for signal handler. */
static int verbose = 0;
static char *serial_port = NULL;
static char *filename = NULL;

static int serial;
static char *line_gcode = NULL;
static FILE *output_file = NULL;


/*
 * Handle SIGTERM.
 */
static void leave(int signal)
{
	if (verbose > 0)
		fprintf(stderr, "dispatcher exiting\n");

	if (output_file)
		fclose(output_file);

	if (serial_port)
		close(serial);

	if (line_gcode)
		free(line_gcode);

	exit(signal);
}


/*
 * Process an internal austerusG control command.
 */
static void process_command(char *line)
{
	if (strncmp(line, MSG_CMD_EXIT, MSG_CMD_EXIT_LEN) == 0)
		leave(EXIT_SUCCESS);
}


int main(int argc, char* argv[])
{
	/* Number of outstanding acknowledgements */
	unsigned int ack_outstanding = 0;

	ssize_t bytes_r, bytes_w;
	size_t line_gcode_len = 0;

	/* Buffer for reading from serial port */
	char line_feedback[SERIAL_LINE_LEN];

	/* User options */
	int baudrate		= DEFAULT_BAUDRATE;
	int serial_timeout	= DEFAULT_TIMEOUT;
	unsigned int ack_count	= DEFAULT_ACKCOUNT;

#ifdef SETUID
	uid_t ruid = getuid();
	uid_t euid = geteuid();

	if (setuid(0) == -1) {
		perror("Error: setuid");
		leave(EXIT_FAILURE);
	}

	euid = geteuid();

	if (euid != 0) {
		perror("Error: setuid");
		leave(EXIT_FAILURE);
	}

	if (nice(-10) == -1) {
		perror("Error: setuid");
		leave(EXIT_FAILURE);
	}

	if (ruid != euid && setuid(ruid) == -1) {
		perror("Error: setuid");
		leave(EXIT_FAILURE);
	}
#endif

	/* Bind to SIGINT for cleanup */
	signal(SIGINT, leave);

	/* Read environmental variables */
	filename	= getenv("AG_DUMP");
	serial_port	= getenv("AG_SERIALPORT");

	/* Allow special NULL string to disable serial port for testing. */
	if (serial_port) {
		if (strcmp(serial_port, "NULL") == 0)
			serial_port = NULL;
	}

	if (getenv("AG_BAUDRATE"))
		baudrate = strtol(getenv("AG_BAUDRATE"), NULL, 10);

	if (getenv("AG_ACKCOUNT"))
		ack_count = strtol(getenv("AG_ACKCOUNT"), NULL, 10);

	if (getenv("AG_VERBOSE"))
		verbose = strtol(getenv("AG_VERBOSE"), NULL, 10);

	if (verbose > 0)
		fprintf(stderr, "verbose mode\n");

	/* Initalise serial port if required */
	if (serial_port) {
		if (verbose > 0)
			fprintf(stderr, "opening serial port %s\n",
								serial_port);

		serial = serial_init(serial_port, baudrate);

		if (serial == -1) {
			perror("Error: unable to open serial port");
			return EXIT_FAILURE;
		}

		usleep(SERIAL_INIT_PAUSE);
		tcflush(serial, TCIOFLUSH);
		usleep(SERIAL_INIT_PAUSE);

		/* Read initialisation message */
		bytes_r = serial_getline(serial, line_feedback, serial_timeout);

		if (bytes_r == -1) {
			perror("Error: serial port timeout");
			return EXIT_FAILURE;
		}

		printf("%s\n", line_feedback);

		usleep(SERIAL_INIT_PAUSE);
		tcflush(serial, TCIOFLUSH);
		usleep(SERIAL_INIT_PAUSE);
	}

	/* Open output file if required */
	if (filename) {
		output_file = fopen(filename, "a");
		if (output_file == NULL) {
			perror("Error: unable to open output file");
			close(serial);
			return EXIT_FAILURE;
		}

		if (verbose > 0)
			fprintf(stderr, "output to %s\n", filename);
	}

	if (verbose > 0)
		fprintf(stderr, "ready\n");

	/* Start of main communications loop */
	while (1) {

		/*
		 * Keep sending lines until we reach the maximum outstanding
		 * acknoledgements.
		 */
		while (ack_outstanding < ack_count || ack_count == 0) {

			/* Block until we have a complete line from stdin */
			bytes_r = getline(&line_gcode, &line_gcode_len, stdin);

			/* Check if stream is closed */
			if (bytes_r == -1)
				leave(EXIT_SUCCESS);

			/* Handle internal austerusG control commands */
			if (strncmp(line_gcode, MSG_CMD, MSG_CMD_LEN) == 0) {
				process_command(line_gcode);
				continue;
			}

			if (output_file) {
				fprintf(output_file, "%s", line_gcode);
				fflush(output_file);
			}

			if (serial_port) {
				/* Don't send empty lines */
				if (bytes_r <= 1)
					continue;

				/* Write the line to the serial port */
				bytes_w = write(serial, line_gcode, bytes_r);
				if (bytes_w != bytes_r) {
					perror("Error: write error");
					leave(EXIT_FAILURE);
				}
			}

			ack_outstanding++;
		}

		/*
		 * We must wait for at least one acknowledgement before we
		 * can send any more lines.
		 */

		while (ack_outstanding >= ack_count && ack_count > 0) {
			/*
			 * Block until we have read an entire line from
			 * serial or we timeout.
			 */
			bytes_r = serial_getline(serial, line_feedback, serial_timeout);

			if (bytes_r == -1) {
				perror("Error: serial timeout, clearing serial buffer");
				tcflush(serial, TCIOFLUSH);
				ack_outstanding = 0;

			} else if (strncmp(line_feedback, MSG_ACK, MSG_ACK_LEN) == 0 ||
					strncmp(line_feedback, MSG_DUD, MSG_DUD_LEN) == 0) {
				/*
				 * If the line is an ACK (either ok or error)
				 * then decrement count and send to stdout.
				 */

				ack_outstanding--;
				printf("%s\n", line_feedback);
				fflush(stdout);

			} else {
				/*
				 * If the line is not an ACK then just send
				 * to stdout.
				 */
				printf("%s\n", line_feedback);
				fflush(stdout);
			}
		}
	}
}
