#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <form.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/wait.h>


#include "austerus-panel.h"
#include "machine.h"
#include "popen2.h"
#include "nbgetline.h"


// Draw a box
void print_box(int x, int y, int w, int h, char *label, int bold) {
	int i, j;

	if (bold > 0)
		attron(A_BOLD);

	move(y, x);
	addch(ACS_ULCORNER);

	for (i=0; i<w-2; i++)
		addch(ACS_HLINE);

	addch(ACS_URCORNER);

	for (i=0; i<h-2; i++) {
		move(y + i + 1, x);
		addch(ACS_VLINE);

		for (j=0; j<w-2; j++)
			printw(" ");

		addch(ACS_VLINE);
	}

	move(y + h - 1, x);
	addch(ACS_LLCORNER);

	for (i=0; i<w-2; i++)
		addch(ACS_HLINE);

	addch(ACS_LRCORNER);

	if (bold > 0)
		attron(A_REVERSE);

	mvprintw(y + (h / 2), x + 1, label);

	if (bold > 0) {
		attroff(A_BOLD);
		attroff(A_REVERSE);
	}
}


// Draw home, end and arrow keys etc
void print_keys(int x, int y, int mask) {
	print_box(x, y, 4, 3, "  ", 0);
	print_box(x + 3, y, 4, 3, "H ", mask & BIT_H);
	print_box(x + 6, y, 4, 3, "+Z", mask & BIT_PZ);
	print_box(x, y + 2, 4, 3, "  ", 0);
	print_box(x + 3, y + 2, 4, 3, "E ", mask & BIT_END);
	print_box(x + 6, y + 2, 4, 3, "-Z", mask & BIT_MZ);

	print_box(x + 3, y + 5, 4, 3, "+Y", mask & BIT_PY);
	print_box(x, y + 7, 4, 3, "-X", mask & BIT_MX);
	print_box(x + 3, y + 7, 4, 3, "-Y", mask & BIT_MY);
	print_box(x + 6, y + 7, 4, 3, "+X", mask & BIT_PX);
}


// Draw the numerical keys
void print_fkeys(int x, int y) {
	print_box(x, y, 4, 3, "1", 0);
	print_box(x + 3, y, 4, 3, "2", 0);

	print_box(x, y + 2, 4, 3, "3", 0);
	print_box(x + 3, y + 2, 4, 3, "4", 0);

	print_box(x, y + 4, 4, 3, "5", 0);
	print_box(x + 3, y + 4, 4, 3, "6", 0);

	print_box(x, y + 6, 4, 3, "7", 0);
	print_box(x + 3, y + 6, 4, 3, "8", 0);

	// TEMP
	print_box(x, y + 8, 4, 3, "f", 0);
	print_box(x + 3, y + 8, 4, 3, "g", 0);
}


void print_extruder_keys(int mask) {
	int x = PANEL_POS_EXTRUDER_X;
	int y = PANEL_POS_EXTRUDER_Y;
	print_box(x, y, 4, 3, "E", mask & BIT_E);
	print_box(x + 3, y, 4, 3, "R", mask & BIT_R);
	print_box(x + 1, y + 2, 4, 3, "D", mask & BIT_D);
}


// Draw the temperature
void print_temperature(unsigned int temp_current, unsigned int temp_target) {
	if (temp_current)
		mvprintw(2, 0, "Temperature  %3.1d^C / %3.1d^C", temp_current, temp_target);
	else
		mvprintw(2, 0, "Temperature    n/a / %3.1d^C", temp_target);
}


// Draw the feedrate
void print_feedrate(unsigned int feedrate) {
	mvprintw(4, 0, "Feedrate     %11.1dmm", feedrate);
}


// Draw the jog distance
void print_jog_distance(unsigned int jog_distance) {
	mvprintw(6, 0, "Jog distance %11.1dmm", jog_distance);
}


// Draw the jog speed
void print_jog_speed(unsigned int jog_speed) {
	mvprintw(8, 0, "Jog speed    %13.1d", jog_speed);
}


// Draw the fan status
void print_fan(unsigned int mode) {
	if (mode)
		mvprintw(10, 0, "Fan                   On ", mode);
	else
		mvprintw(10, 0, "Fan                   Off", mode);
}


// TODO we are not adjusting posX etc here!
// Send one or all axes to home
int home_axis(FILE *stream_gcode, char axis) {
	switch (axis) {
		case 'x':
		case 'X':
			mvprintw(LINES - 1, 0, "Status: Home x  ");
			fprintf(stream_gcode, "G28 X0\n");
			fflush(stream_gcode);
			break;
		case 'y':
		case 'Y':
			mvprintw(LINES - 1, 0, "Status: Home y  ");
			fprintf(stream_gcode, "G28 Y0\n");
			fflush(stream_gcode);
			break;
		case 'z':
		case 'Z':
			mvprintw(LINES - 1, 0, "Status: Home z  ");
			fprintf(stream_gcode, "G28 Z0\n");
			fflush(stream_gcode);
			break;
		case 'a':
		case 'A':
			mvprintw(LINES - 1, 0, "Status: Home all");
			fprintf(stream_gcode, "G28\n");
			fflush(stream_gcode);
			break;
		default:
			return 0;
	}
	return 1;
}


// TODO we are not adjusting posX etc here!
// Send one or all axes to end
int end_axis(FILE *stream_gcode, char axis) {
	switch (axis) {
		case 'x':
		case 'X':
			mvprintw(LINES - 1, 0, "Status: End x   ");
			fprintf(stream_gcode, "G1 X%d\n", MAX_X + OVER_LIMIT);
			fflush(stream_gcode);
			break;
		case 'y':
		case 'Y':
			mvprintw(LINES - 1, 0, "Status: End y   ");
			fprintf(stream_gcode, "G1 Y%d\n", MAX_Y + OVER_LIMIT);
			fflush(stream_gcode);
			break;
		case 'z':
		case 'Z':
			mvprintw(LINES - 1, 0, "Status: End z  q ");
			fprintf(stream_gcode, "G1 Z%d\n", MAX_Z + OVER_LIMIT);
			fflush(stream_gcode);
			break;
		case 'a':
		case 'A':
			mvprintw(LINES - 1, 0, "Status: End all ");
			fprintf(stream_gcode, "G1 X%d Y%d Z%d\n",
				MAX_X + OVER_LIMIT,
				MAX_Y + OVER_LIMIT,
				MAX_Z + OVER_LIMIT
			);
			fflush(stream_gcode);
			break;
		default:
			return 0;
	}
	return 1;
}


// Print usage to terminal
void usage(void) {
	printf("Usage: austerus-panel [OPTION]...\n"
	"\n"
	"Options:\n"
	" -h, --help             Print this help message\n"
	" -p, --port=serialport  Serial port Arduino is on\n"
	" -b, --baud=baudrate    Baudrate (bps) of Arduino\n"
	" -v, --verbose          Print extra output\n"
	"\n");
}


int main(int argc, char* argv[]) {
	int nbytes;
	int status;
	unsigned int previous, ch;
	unsigned int temp_extruder = 0, temp_bed = 0;
	// rename TODO
	unsigned int key_mask=0;

	unsigned int jog_distance = DEFAULT_JOG_DISTANCE;
	unsigned int jog_speed = DEFAULT_JOG_SPEED;
	unsigned int temp_target = DEFAULT_TEMP_TARGET;
	unsigned int feedrate = DEFAULT_FEEDRATE;
	// Explain?
	float delta_e = 0.001666669999999968 * (float)feedrate;;

	// TODO heater on/off keys

	// TODO consider a struct to hold state, then we can declare it once
	// and pass a single pointer around to various functions!!!

	// TODO float? need to?
	int posX=0, posY=0, posZ=0;
	float posE=0.0;
	int extruding=0;

	time_t last_temp = time(NULL);
	time_t last_extrude = time(NULL);

	int pipe_gcode = 0, pipe_feedback = 0;
	//stream_gcode = fdopen(pipe_gcode[1], "w");

	FILE *stream_gcode = NULL, *stream_feedback = NULL;

	// User options
	char *serial_port = NULL;

	char *cmd = NULL;	// Command string to execute austerus-core

	//size_t line_feedback_len = PIPE_LINE_BUFFER_LEN;
	//char *line_feedback = NULL;
	char line_feedback[LINE_FEEDBACK_LEN];

	// Allocate inital size of input line buffer
	//pipe_buffer = (char *) malloc (pipe_buffer_len + 1);

	// Read command line options
	int option_index = 0, opt=0;
	static struct option loptions[] = {
		{"help", no_argument, 0, 'h'},
		{"port", required_argument, 0, 'p'},
		{"baud", required_argument, 0, 'b'},
		{"verbose", no_argument, 0, 'v'}
	};

	// Generate the command line for austerus-core
	asprintf(&cmd, "/usr/bin/env AG_ACKCOUNT=1 PATH=$PWD:$PATH");

	while(opt >= 0) {
		opt = getopt_long(argc, argv, "hp:b:v", loptions,
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
			case 'v':
				asprintf(&cmd, "%s AG_VERBOSE=1", cmd);
				break;
		}
	}

	if (!serial_port & !getenv("AG_SERIALPORT")) {
		fprintf(stderr, "A serial port must be specified\n");
		return EXIT_FAILURE;
	}

	asprintf(&cmd, "%s austerus-core", cmd);

	// Open the gcode output stream to austerus-core
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

	// Start curses mode
	initscr();
	// Line buffering disabled
	raw();
	//  We get F1, F2 etc
	keypad(stdscr, TRUE);
	// Don't echo() while we do getch
	noecho();
	// Hide cursor
	curs_set(0);

	// Timeout so we can run extruder and check for feedback
	timeout(CURSES_TIMEOUT);

	// draw initial screen
	mvprintw(0, 0, "austerusG %s", VERSION);
	mvprintw(0, 30, "- +");

	print_fkeys(PANEL_POS_NUMBERS_X, PANEL_POS_NUMBERS_Y);
	print_extruder_keys(key_mask);
	print_keys(PANEL_POS_KEYS_X, PANEL_POS_KEYS_Y, key_mask);

	print_temperature(temp_extruder, temp_target);
	print_feedrate(feedrate);
	print_jog_distance(jog_distance);
	print_jog_speed(jog_speed);
	print_fan(0);

	mvprintw(LINES - 1, 0, "Status: Idle");

	refresh();

	// start of print reset
	fprintf(stream_gcode, "M110\n");
	// set absolute positioning
	fprintf(stream_gcode, "G90\n");
	// set to mm
	fprintf(stream_gcode, "G21\n");
	// reset coordinates to zero
	fprintf(stream_gcode, "G92 X0 Y0 Z0 E0\n");
	fflush(stream_gcode);

	while (1) {
		// Wait for user input
		ch = getch();

		// Handle any two key sequences
		switch (previous) {
			case KEY_HOME:
				if (home_axis(stream_gcode, ch)) {
					key_mask = key_mask & !BIT_H;
					print_keys(PANEL_POS_KEYS_X, PANEL_POS_KEYS_Y, key_mask);
				}
				break;
			case KEY_END:
				if (end_axis(stream_gcode, ch)) {
					key_mask = key_mask & !BIT_END;
					print_keys(PANEL_POS_KEYS_X, PANEL_POS_KEYS_Y, key_mask);
				}
				break;
		}

		// Handle single key press
		switch (ch) {
			case '1':
				// Decrease target temperature
				if (temp_target > 0) {
					temp_target = temp_target - 1;

					fprintf(stream_gcode, "M104 S%d\n", temp_target);
					fflush(stream_gcode);

					print_temperature(temp_extruder, temp_target);
				}
				break;

			case '2':
				// Increase target temperature
				temp_target = temp_target + 1;

				fprintf(stream_gcode, "M104 S%d\n", temp_target);
				fflush(stream_gcode);

				print_temperature(temp_extruder, temp_target);
				break;

			case '3':
				// Decrease feedrate
				if (feedrate >= 1) {
					feedrate -= 1;
					delta_e = 0.001666669999999968 * (float)feedrate;
					print_feedrate(feedrate);
				}
				break;

			case '4':
				// Increase feedrate
				feedrate += 1;
				delta_e = 0.001666669999999968 * (float)feedrate;
				print_feedrate(feedrate);
				break;

			case '5':
				// Decrease jog distance
				if (jog_distance > 10) {
					fprintf(stream_gcode, "M104 %d\n", temp_target);
					fflush(stream_gcode);
					jog_distance = jog_distance - 10;
					print_jog_distance(jog_distance);
				}
				break;

			case '6':
				// Increase jog distance
				jog_distance = jog_distance + 10;
				print_jog_distance(jog_distance);
				break;

			case '7':
				// Decrease jog speed
				if (jog_speed > 10) {
					jog_speed = jog_speed - 100;
					print_jog_speed(jog_speed);
				}
				break;

			case '8':
				// Increase jog speed
				jog_speed = jog_speed + 100;
				print_jog_speed(jog_speed);
				break;

			case 'e':
				extruding = 1;
				key_mask = (key_mask | BIT_E) & ~(BIT_D | BIT_R);
				print_extruder_keys(key_mask);
				break;

			case 'd':
				extruding = 0;
				key_mask = (key_mask | BIT_D) & ~(BIT_E | BIT_R);
				print_extruder_keys(key_mask);
				break;

			case 'r':
				extruding = -1;
				key_mask = (key_mask | BIT_R) & ~(BIT_D | BIT_E);
				print_extruder_keys(key_mask);
				break;

			case KEY_LEFT:
				// Jog X axis minus
				// TODO merits of G0 or G1 moves?
				posX -= jog_distance;
				fprintf(
					stream_gcode, "G1 X%d Y%d Z%d F%d\n",
					posX, posY, posZ, jog_speed
				);
				fflush(stream_gcode);
				key_mask = key_mask | BIT_MX;
				print_keys(PANEL_POS_KEYS_X, PANEL_POS_KEYS_Y, key_mask);
				break;

			case KEY_RIGHT:
				// Jog X axis plus
				posX += jog_distance;
				fprintf(
					stream_gcode, "G1 X%d Y%d Z%d F%d\n",
					posX, posY, posZ, jog_speed
				);
				fflush(stream_gcode);
				key_mask = key_mask | BIT_PX;
				print_keys(PANEL_POS_KEYS_X, PANEL_POS_KEYS_Y, key_mask);
				break;

			case KEY_UP:
				// Jog Y axis plus
				posY += jog_distance;
				fprintf(
					stream_gcode, "G1 X%d Y%d Z%d F%d\n",
					posX, posY, posZ, jog_speed
				);
				fflush(stream_gcode);
				key_mask = key_mask | BIT_PY;
				print_keys(PANEL_POS_KEYS_X, PANEL_POS_KEYS_Y, key_mask);
				break;

			case KEY_DOWN:
				// Jog Y axis minus
				posY -= jog_distance;
				fprintf(
					stream_gcode, "G1 X%d Y%d Z%d F%d\n",
					posX, posY, posZ, jog_speed
				);
				fflush(stream_gcode);
				key_mask = key_mask | BIT_MY;
				print_keys(PANEL_POS_KEYS_X, PANEL_POS_KEYS_Y, key_mask);
				break;

			case KEY_PPAGE:
				// Jog Z axis up
				posZ += jog_distance;
				fprintf(
					stream_gcode, "G1 X%d Y%d Z%d F%d\n",
					posX, posY, posZ, jog_speed
				);
				fflush(stream_gcode);
				key_mask = key_mask | BIT_PZ;
				print_keys(PANEL_POS_KEYS_X, PANEL_POS_KEYS_Y, key_mask);
				break;

			case KEY_NPAGE:
				// Jog Z axis down
				posZ -= jog_distance;
				fprintf(
					stream_gcode, "G1 X%d Y%d Z%d F%d\n",
					posX, posY, posZ, jog_speed
				);
				fflush(stream_gcode);
				key_mask = key_mask | BIT_MZ;
				print_keys(PANEL_POS_KEYS_X, PANEL_POS_KEYS_Y, key_mask);
				break;

			case KEY_HOME:
				// Start of two key move to home command
				key_mask = key_mask | BIT_H;
				print_keys(PANEL_POS_KEYS_X, PANEL_POS_KEYS_Y, key_mask);
				previous = ch;
				break;

			case KEY_END:
				// Start of two key move to end command
				key_mask = key_mask | BIT_END;
				print_keys(PANEL_POS_KEYS_X, PANEL_POS_KEYS_Y, key_mask);
				previous = ch;
				break;

			case 'f':
			case 'F':
				// Fan on
				fprintf(stream_gcode, "M106\n");
				fflush(stream_gcode);
				print_fan(1);

				break;

			case 'g':
			case 'G':
				// Fan off
				fprintf(stream_gcode, "M107\n");
				fflush(stream_gcode);
				print_fan(0);

				break;

			case 'q':
			case KEY_ESC:
				// Shutdown printer
				fprintf(stream_gcode, "M112\n");
				// Exit core
				fprintf(stream_gcode, "#ag:exit\n\n");
				fflush(stream_gcode);

				endwin();

				pclose(stream_gcode);
				pclose(stream_feedback);

				close(pipe_gcode);
				close(pipe_feedback);

				//if (line_feedback)
				//	free(line_feedback);

				wait(&status);
				printf("core exit = %d\n", status);

				return EXIT_SUCCESS;
		}

		// TODO check core alive?

		if (time(NULL) - last_temp > TEMP_PERIOD)
		{
			do {
				nbytes = nonblock_getline(line_feedback, stream_feedback);
				
				//nbytes = getline(&line_feedback, &line_feedback_len, stream_feedback);

				//fprintf(stream_gcode, "NB %d %s\n", nbytes);
				//fflush(stream_gcode);

				if (nbytes > 0) {

					mvprintw(LINES - 3, 0, "nb %d", nbytes);
					sscanf(line_feedback, "ok T:%d B:%d", &temp_extruder, &temp_bed);

					//
					if (nbytes > 66) {
						line_feedback[63] = '.';
						line_feedback[64] = '.';
						line_feedback[65] = '.';
						line_feedback[66] = '\0';
					}

					mvprintw(LINES - 2, 0, "Response: %s", line_feedback);
					print_temperature(temp_extruder, temp_target);
				}

			} while (nbytes != -1);


			last_temp = time(NULL);

			fprintf(stream_gcode, "M105\n");
			fflush(stream_gcode);

			// tODO this needs it's own clock too!
			// tODO arrow keys etc
			if (key_mask |  BIT_D) {
				key_mask = key_mask & ~BIT_D;
				print_extruder_keys(key_mask);
			}

			// We can make one mask #def? from all keys we want cleared
			// and use it for check andf clear!


		}


		if (time(NULL) - last_extrude > EXTRUDE_PERIOD)
		{
			last_extrude = time(NULL);

			// This needs to be on an indepentent time check so it can be controlled!! TODO
			if (extruding > 0) {
				posE += delta_e;
				fprintf(stream_gcode, "G1 E%.2f F%d\n", posE, feedrate);
				fflush(stream_gcode);
			} else if (extruding < 0) {
				posE -= delta_e;
				fprintf(stream_gcode, "G1 E%.2f F%d\n", posE, feedrate);
				fflush(stream_gcode);
			}

			// tODO arrow keys etc
			if (key_mask |  BIT_D) {
				key_mask = key_mask & ~BIT_D;
				print_extruder_keys(key_mask);
			}

			// We can make one mask #def? from all keys we want cleared
			// and use it for check andf clear!
		}
		refresh();


		// TODO check core is still alive

	}
}


