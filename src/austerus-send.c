#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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

	print_time(taken);
	printf("  ");
	print_time(estimate - taken);
}


void print_file(FILE *stream_gcode, FILE *stream_input) {
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
		printf("%s", line);
	}

	if (line)
		free(line);
}


int main(int argc, char *argv[])
{
	FILE *stream_gcode;
	FILE *stream_input;
	int status;
	int i;

	stream_gcode = popen(
		"/usr/bin/env PATH=$PWD:$PATH "
		"austerus-core -p /dev/ttyACM0 -b 57600 -c 2 ", "w"
	);

	if (!stream_gcode)
	{
		fprintf(stderr, "incorrect parameters or too many files.\n");
		return EXIT_FAILURE;
	}

	for (i=1; i<argc; i++) {
		stream_input = fopen(argv[i], "r");

		if (stream_input == NULL) {
			fprintf(stderr, "file error\n");
			return EXIT_FAILURE;
		}

		printf("printing file %s\n", argv[i]);
		print_file(stream_gcode, stream_input);
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




