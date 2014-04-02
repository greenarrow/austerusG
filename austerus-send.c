#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#include "popen2.h"
#include "nbgetline.h"
#include "stats.h"
#include "protocol.h"
#include "austerus-send.h"


// Print a duration time given in seconds in the most appropriate units.
void print_duration(time_t time)
{
	time_t remain = time;
	int hours;
	int mins;

	hours = remain / 60 / 60;
	remain -= (hours * 60 * 60);
	mins = remain / 60;
	remain -= (mins * 60);

	if (hours >= 1) {
		printf("%dh %dm", hours, mins);
		return;
	}

	if (mins >= 1) {
		printf("%dm", mins);
		return;
	}

	printf("%lds", remain);
}


// Print the status line to the console
void print_status(int pct, int taken, int estimate) {
	int i, j;

	printf("%3d%%[", pct);

	for(i=0;i<(int)((float)(BAR_WIDTH - 7) * (float)pct / 100.0);i++)
		printf("=");

	printf(">");

	for(j=0;j<BAR_WIDTH - i - 7;j++)
		printf(" ");

	taken = estimate * pct / 100;
	printf("] ");
}


// Print the status line to the console
void print_status_stream(int pct, time_t start) {
	int taken;

	taken = time(NULL) - start;

	printf("%d%% complete (", pct);

	if (pct == 0)
		printf("unknown");
	else
		print_duration((taken * 100 / pct) - taken);

	printf(" remaining)\n");
}


// Strip out comments from line
ssize_t filter_comments(char *line) {
	int i = 0;
	for (i=0; i<strlen(line) - 1; i++) {
		if (line[i] == '(' || line[i] == ';') {
			line[i] = '\n';
			line[i + 1] = '\0';
		}
	}
	return i;
}


// Print gcode from stream_input to austerus-core on stream_gcode
int print_file(FILE *stream_input, size_t lines, const char *cmd,
	unsigned int filament, unsigned int *table, int mode, int verbose) {

	int pipe_gcode = 0;
	int pipe_feedback = 0;

	FILE *stream_gcode = NULL;
	FILE *stream_feedback = NULL;

	int status;
	time_t start;

	int i;

	char *line = NULL;
	size_t line_len = 0;
	ssize_t nbytes;
	ssize_t fbytes;
	size_t tally = 0;
	char line_feedback[1024];

	int pcta = -1, pctb = 0;

	pid_t pid;

	start = time(NULL);

	// Open the input and output streams to austerus-core
	pid = popen2(cmd, &pipe_gcode, &pipe_feedback);

	// Make feedback pipe non-blocking
	fcntl(pipe_feedback, F_SETFL, O_NONBLOCK);

	stream_gcode = fdopen(pipe_gcode, "w");
	stream_feedback = fdopen(pipe_feedback, "r");

	if (!stream_gcode) {
		fprintf(stderr, "unable to open output stream\n");
		abort();
	}

	if (!stream_feedback) {
		fprintf(stderr, "unable to open feedback stream\n");
		abort();
	}

	if (mode == NORMAL) {
		for(i=0; i<BAR_WIDTH; i++)
			printf(" ");
	}

	while (nbytes != -1) {
		// Read the next line from file
		nbytes = getline(&line, &line_len, stream_input);
		if (nbytes < 0)
			break;

		// Strip out any comments
		nbytes = filter_comments(line);

		if (nbytes == 0)
			continue;

		if (nbytes == 1 && line[0] == '\n')
			continue;

		if (nbytes == 2 && line[0] == '\t' && line[1] == '\n')
			continue;

		// Write the file to the core
		fprintf(stream_gcode, "%s", line);
		fflush(stream_gcode);

		if (verbose)
			printf("SEND: %s", line);

		// Read any available feedback lines
		do {
			fbytes = nonblock_getline(line_feedback,
				stream_feedback);
			if (fbytes > 0) {
				if (strncmp(line_feedback, MSG_ACK,
						MSG_ACK_LEN) == 0 ||
					strncmp(line_feedback, MSG_DUD,
						MSG_DUD_LEN) == 0) {
					tally++;
				}
				if (verbose)
					printf("FEEDBACK: %s\n",
						line_feedback);
			}
		} while (fbytes != -1);

		if (tally > lines) {
			fprintf(stderr, "Expected %lu valid lines, got more\n",
				(long unsigned int) lines);
			return 0;
		}

		if (filament == 0)
			pctb = 0;
		else
			pctb = 100 * table[tally] / filament;

		if (pcta != pctb) {
			pcta = pctb;

			if (mode == NORMAL) {
				printf("\r");
				print_status(pcta, 0, 0);
			} else {
				print_status_stream(pcta, start);
			}

			fflush(stdout);
		}
	}

	if (mode == NORMAL)
		printf("\n");

	/*
	 * Now we have written and flushed all outgoing gcode we can close the
	 * pipe leaving the core to finish reading the data.
	 */

	// Block until stream is closed
	if (pclose(stream_gcode) != 0)
		perror("error closing stream");

	close(pipe_gcode);

	if (wait(&status) != pid)
		perror("error waiting for core");

	status = WEXITSTATUS(status);

	/*
	 * Read any remaining data from the feedback pipe until the core has
	 * exited.
	 */

	while (1) {
		fbytes = nonblock_getline(line_feedback, stream_feedback);
		if (fbytes == -1) {
			if (kill(pid, 0) == 0)
				usleep(100);
			else
				break;
		}

		if (fbytes > 0) {
			if (strncmp(line_feedback, MSG_ACK,
					MSG_ACK_LEN) == 0 ||
				strncmp(line_feedback, MSG_DUD,
					MSG_DUD_LEN) == 0) {
				tally++;
			}
			if (verbose)
				printf("FEEDBACK (post): %s\n", line_feedback);

			/*
			 * TODO We should still be updating progress here */
		}
	}

	if (tally != lines) {
		fprintf(stderr, "Expected %lu valid lines, got more %lu\n",
			(long unsigned int) lines, (long unsigned int) tally);
	}

	if (pclose(stream_feedback) != 0)
		perror("error closing stream");

	if (line)
		free(line);

	close(pipe_feedback);

	return status;
}


// Print usage to terminal
void usage(void) {
	printf("Usage: austerus-send [OPTION]... [FILE]...\n"
	"\n"
	"Options:\n"
	" -h, --help             Print this help message\n"
	" -p, --port=serialport  Serial port Arduino is on\n"
	" -b, --baud=baudrate    Baudrate (bps) of Arduino\n"
	" -c, --ack-count        Set delayed ack count (1 is no delayed ack)\n"
	" -s, --stream           Run in stream mode\n"
	" -v, --verbose          Print extra output\n"
	"\n");
}


int main(int argc, char *argv[])
{
	FILE *stream_input;

	char *serial_port = NULL;
	int mode = NORMAL;
	int verbose = 0;
	int status = 0;
	int rc = 0;

	char *cmd = NULL;	// Command string to execute austerus-core

	int i;

	unsigned int *table = NULL;
	size_t lines = 0;
	float filament = 0.0;

	// Read command line options
	int option_index = 0, opt=0;
	static struct option loptions[] = {
		{"help", no_argument, 0, 'h'},
		{"port", required_argument, 0, 'p'},
		{"baud", required_argument, 0, 'b'},
		{"ack-count", required_argument, 0, 'c'},
		{"stream", no_argument, 0, 's'},
		{"verbose", no_argument, 0, 'v'}
	};

	// Generate the command line for austerus-core
	asprintf(&cmd, "/usr/bin/env PATH=$PWD:$PATH");

	while(opt >= 0) {
		opt = getopt_long(argc, argv, "hp:b:c:sv", loptions,
			&option_index);

		switch (opt) {
			case 'h':
				usage();
				return EXIT_SUCCESS;
			case 'p':
				serial_port = optarg;
				asprintf(&cmd, "%s AG_SERIALPORT=%s", cmd,
					optarg);
				break;
			case 'b':
				asprintf(&cmd, "%s AG_BAUDRATE=%ld", cmd,
					strtol(optarg, NULL, 10));
				break;
			case 'c':
				asprintf(&cmd, "%s AG_ACKCOUNT=%ld", cmd,
					strtol(optarg, NULL, 10));
				break;
			case 's':
				mode = STREAM;
				break;
			case 'v':
				verbose = 1;
				asprintf(&cmd, "%s AG_VERBOSE=1", cmd);
				break;
		}
	}

	if (!serial_port & !getenv("AG_SERIALPORT")) {
		fprintf(stderr, "A serial port must be specified\n");
		return EXIT_FAILURE;
	}

	asprintf(&cmd, "%s austerus-core", cmd);

	for (i=optind; i<argc; i++) {
		printf("starting print: %s\n", argv[i]);
		fflush(stdout);

		stream_input = fopen(argv[i], "r");

		if (stream_input == NULL) {
			fprintf(stderr, "file error\n");
			return EXIT_FAILURE;
		}

		filament = get_progress_table(&table, &lines, stream_input);
		if (lines == 0) {
			fprintf(stderr, "file contains no lines\n");
			return EXIT_FAILURE;
		}
		printf("total filament length: %fmm\n", filament);

		rewind(stream_input);
		rc = print_file(stream_input, lines, cmd,
			(unsigned int) filament, table, mode, verbose);

		if (rc != 0) {
			if (rc > status)
				status = rc;

			printf("bad exit from core: %d\n", rc);
		}

		fclose(stream_input);

		printf("completed print: %s\n", argv[i]);

		free(table);
	}

	free(cmd);
	return status;
}

