#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#include "popen2.h"
#include "nbgetline.h"
#include "stats.h"
#include "austerus-send.h"


// Print a time in seconds as HH:MM:SS
void print_time(int seconds) {
	int mins = seconds / 60;
	int hours = seconds / 3600;
	printf("%3.2d:%2.2d:%2.2d", hours, mins, seconds - (mins * 60 + hours * 60));
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
void print_file(FILE *stream_gcode, FILE *stream_feedback, FILE *stream_input,
		size_t lines, unsigned int filament, unsigned int *table,
		int verbose) {
	int i;

	char *line = NULL;
	size_t line_len = 0;
	ssize_t nbytes;
	ssize_t fbytes;
	size_t tally = 0;
	char line_feedback[1024];

	int pcta = 0, pctb = 0;

	for(i=0; i<BAR_WIDTH; i++)
		printf(" ");

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
			printf("%s", line);

		// Read any available feedback lines
		do {
			fbytes = nonblock_getline(line_feedback, stream_feedback);
			if (fbytes > 0 && verbose)
				printf("FEEDBACK: %s\n", line_feedback);
		} while (fbytes != -1);

		if (tally > lines) {
			fprintf(stderr, "Expected %lu valid lines, got more\n",
				(long unsigned int) lines);
			return;
		}

		/*
		 * TODO currently based on lines sent tally but should be based
		 * on count returned ACKs!
		 */
		pctb = 100 * table[tally] / filament;

		if (pcta != pctb) {
			pcta = pctb;

			printf("\r");
			print_status(pcta, 0, 0);
			fflush(stdout);
		}

		tally++;
	}

	printf("\n");

	if (tally != lines) {
		fprintf(stderr, "Expected %lu valid lines, got more %lu\n",
			(long unsigned int) lines, (long unsigned int) tally);
		abort();
	}

	if (line)
		free(line);
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
	" -v, --verbose          Print extra output\n"
	"\n");
}


int main(int argc, char *argv[])
{
	int pipe_gcode = 0, pipe_feedback = 0;

	FILE *stream_gcode;
	FILE *stream_feedback;
	FILE *stream_input;

	char *serial_port = NULL;
	int verbose = 0;

	char *cmd = NULL;	// Command string to execute austerus-core

	int status;
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
		{"verbose", no_argument, 0, 'v'}
	};

	// Generate the command line for austerus-core
	asprintf(&cmd, "/usr/bin/env PATH=$PWD:$PATH");

	while(opt >= 0) {
		opt = getopt_long(argc, argv, "hp:b:c:v", loptions,
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

	// Open the input and output streams to austerus-core
	popen2(cmd, &pipe_gcode, &pipe_feedback);
	free(cmd);

	// Make feedback pipe non-blocking
	fcntl(pipe_feedback, F_SETFL, O_NONBLOCK);

	stream_gcode = fdopen(pipe_gcode, "w");
	stream_feedback = fdopen(pipe_feedback, "r");

	if (!stream_gcode) {
		fprintf(stderr, "unable to open output stream\n");
		return EXIT_FAILURE;
	}

	if (!stream_feedback) {
		fprintf(stderr, "unable to open feedback stream\n");
		return EXIT_FAILURE;
	}

	for (i=optind; i<argc; i++) {
		printf("starting print: %s\n", argv[i]);

		stream_input = fopen(argv[i], "r");

		if (stream_input == NULL) {
			fprintf(stderr, "file error\n");
			return EXIT_FAILURE;
		}

		filament = get_progress_table(&table, &lines, stream_input);
		printf("total filament length: %fmm\n", filament);

		rewind(stream_input);
		print_file(stream_gcode, stream_feedback, stream_input, lines,
				(unsigned int) filament, table, verbose);

		fclose(stream_input);

		printf("completed print: %s\n", argv[i]);

		free(table);
	}

	// Tell core to exit
	fprintf(stream_gcode, "#ag:exit\n");
	fflush(stream_gcode);

	wait(&status);

	if (status != 0)
		printf("bad exit from core: %d\n", status);

	// Block until stream is closed
	if (pclose(stream_gcode) != 0)
		perror("error closing stream");

	if (pclose(stream_feedback) != 0)
		perror("error closing stream");

	close(pipe_gcode);
	close(pipe_feedback);

	return EXIT_SUCCESS;
}




