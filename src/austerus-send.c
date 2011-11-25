#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>

#include "austerus-send.h"
#include "defaults.h"


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

	print_time(taken);
	printf("  ");
	print_time(estimate - taken);
}


// Print gcode from stream_input to austerus-core on stream_gcode
void print_file(FILE *stream_gcode, FILE *stream_input, int verbose) {
	int i;

	char *line = NULL;
	size_t line_len = 0;
	ssize_t nbytes;

	for(i=0; i<BAR_WIDTH; i++)
		printf(" ");

	printf("     taken  remaining\n");

	while (nbytes != -1) {
	
		nbytes = getline(&line, &line_len, stream_input);

		if (line[0] == ';')
			continue;

		if (nbytes <= 0)
			continue;

		fprintf(stream_gcode, "%s", line);
		fflush(stream_gcode);

		if (verbose)
			printf("%s", line);
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
	FILE *stream_gcode;
	FILE *stream_input;

	char *serial_port = NULL;
	unsigned int ack_count = DEFAULT_ACK_COUNT;
	int baudrate = DEFAULT_BAUDRATE;
	int verbose = 0;

	char *cmd = NULL;	// Command string to execute austerus-core

	int status;
	int i;

	// Read command line options
	int option_index = 0, opt=0;
	static struct option loptions[] = {
		{"help", no_argument, 0, 'h'},
		{"port", required_argument, 0, 'p'},
		{"baud", required_argument, 0, 'b'},
		{"ack-count", required_argument, 0, 'c'},
		{"verbose", no_argument, 0, 'v'}
	};

	while(opt >= 0) {
		opt = getopt_long(argc, argv, "hp:b:c:v", loptions,
			&option_index);

		switch (opt) {
			case 'h':
				usage();
				return EXIT_SUCCESS;
			case 'p':
				serial_port = optarg;
				break;
			case 'b':
				baudrate = strtol(optarg, NULL, 10);
				break;
			case 'c':
				ack_count = strtol(optarg, NULL, 10);
				break;
			case 'v':
				verbose = 1;
				break;
		}
	}

	if(!serial_port) {
		fprintf(stderr, "A serial port must be specified\n");
		return EXIT_FAILURE;
	}

	// Generate the command line for austerus-core
	asprintf(&cmd, 
		"/usr/bin/env PATH=$PWD:$PATH austerus-core -p %s -b %d -c %d",
		serial_port, baudrate, ack_count);

	// Open the gcode output stream to austerus-core
	stream_gcode = popen(cmd, "w");
	free(cmd);

	if (!stream_gcode)
	{
		fprintf(stderr, "incorrect parameters or too many files.\n");
		return EXIT_FAILURE;
	}

	for (i=optind; i<argc; i++) {
		stream_input = fopen(argv[i], "r");

		if (stream_input == NULL) {
			fprintf(stderr, "file error\n");
			return EXIT_FAILURE;
		}

		printf("printing file %s\n", argv[i]);
		print_file(stream_gcode, stream_input, verbose);
		fclose(stream_input);
		printf("finished printing file %s\n", argv[i]);
	}

	// Tell core to exit
	fprintf(stream_gcode, "#ag:exit\n");
	fflush(stream_gcode);

	printf("waiting for core to exit\n");

	wait(&status);
	printf("core exit = %d\n", status);

	// Block until stream is closed
	if (pclose(stream_gcode) != 0)
	{
		fprintf(stderr, "error closing stream\n");
	}

	return EXIT_SUCCESS;
}




