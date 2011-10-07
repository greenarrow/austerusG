#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <getopt.h>
#include <sys/ioctl.h>

#include "serial.h"
#include "austerus-core.h"


// User options. Global for signal handler.
int verbose = 0;
char *serial_port = NULL;
char *filename;

int serial;
FILE *output_file;


// Print help
void usage(void) {
	printf("Usage: austerus-core [OPTIONS]\n"
	"\n"
	"Options:\n"
	" -h, --help             Print this help message\n"
	" -p, --port=serialport  Serial port Arduino is on\n"
	" -b, --baud=baudrate    Baudrate (bps) of Arduino\n"
	" -c, --ack-count        Set delayed ack count (1 is no delayed ack)\n"
	" -v, --verbose          Print extra output\n"
	"\n");
}


// Main function
int main(int argc, char* argv[]) {
	int status;
	int nbytes;

	unsigned int ack_count = DEFAULT_ACK_COUNT;
	unsigned int ack_outstanding = 0;
	unsigned int serial_retries;
	unsigned int buffer_len;

	size_t pipe_buffer_len = PIPE_LINE_BUFFER_LEN;

	char serial_buffer[SERIAL_LINE_BUFFER_LEN];
	char *pipe_buffer;

	// User options
	int baudrate = DEFAULT_BAUDRATE;

	// Display help
	if (argc==1) {
		usage();
		return EXIT_SUCCESS;
	}

	// Bind to SIGINT for cleanup
	signal(SIGINT, leave);

	// Read environmental variables
	filename = getenv("ASG_OUTPUT_FILENAME");

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

	// Allocate inital size of input line buffer
	pipe_buffer = (char *) malloc (pipe_buffer_len + 1);

	if (verbose > 0)
		printf("verbose mode\n");

	// Initalise serial port if required
	if (serial_port) {

		if (verbose > 0)
			printf("opening serial port %s\n", serial_port);

		serial = serial_init(serial_port, baudrate);
		
		if (serial == -1) {
			perror("Error: unable to open serial port");
 			return EXIT_FAILURE;
		}

		usleep(SERIAL_INIT_PAUSE);
		tcflush(serial, TCIOFLUSH);
		usleep(SERIAL_INIT_PAUSE);

		nbytes = serial_getline(serial, serial_buffer);
		printf("%s", serial_buffer);

		tcflush(serial, TCIOFLUSH);
		usleep(SERIAL_INIT_PAUSE);
	}

	// Open output file if required
	if (filename) {
		output_file = fopen(filename, "a");
		if (output_file == NULL) {
			perror("Error: unable to open output file");
			close(serial);
			return EXIT_FAILURE;
		}

		if (verbose > 0)
			printf("output to %s\n", filename);
	}

	if (verbose > 0)
		printf("ready\n");

	// Start of main communications loop
	while (1) {

		// Keep sending lines until we reach the maximum outstanding
		// acknoledgements
		while (ack_outstanding < ack_count || ack_count == 0) {

			// Block until we have a complete line from stdin
			nbytes = getline(&pipe_buffer, &pipe_buffer_len, stdin);

			if (nbytes == -1) {
				perror("Error: stream error");
				fprintf(output_file, "Error: stream error\n");
				leave(EXIT_FAILURE);
			}

			if (filename) {
				fprintf(output_file, "%s", pipe_buffer);
				fflush(output_file);
			}

			if (serial_port) {
				// Write the line to the serial port
				buffer_len = strlen(pipe_buffer);
				nbytes = write(serial, pipe_buffer, buffer_len);
				if(nbytes != buffer_len) {
					perror("Error: write error");
					leave(EXIT_FAILURE);
				}
			}

			ack_outstanding++;
		}

		// We must wait for at least one acknowledgement before we can
		// send any more lines.
		serial_retries = 0;

		while (ack_outstanding >= ack_count && ack_count > 0) {
			// Block until we have read an entire line from serial
			// or we timeout
			nbytes = serial_getline(serial, serial_buffer);

			if (nbytes == -1) {
				perror("Warning: ACK has not been received");
				serial_retries++;

				// After a defined number of attempts
				// (timeouts) we give up waiting for the ACKs
				// and clear the buffer. This prevents
				// deadlocks from occuring.

				if (serial_retries > SERIAL_RETRIES) {
					perror("Error: Out of retries, clearing serial buffer");
					ack_outstanding = 0;
					tcflush(serial, TCIOFLUSH);
				}

			} else if (strncmp(serial_buffer, MSG_ACK, nbytes) == 0) {
				ack_outstanding--;
				printf("%s\n", serial_buffer);

			// TODO also accept line error response to decrement
			// counter

			} else {
				// Line is not an ACK so send to stdout
				printf("%s\n", serial_buffer);
			}
		}
	}
}


// Handle SIGTERM
void leave(int signal) {
	if (verbose > 0)
		printf("dispatcher exiting\n");

	if (filename)
		fclose(output_file);

	if (serial_port)
		close(serial);

	exit(signal);
}


