#define _GNU_SOURCE /* ssize_t */
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>


/*
 * Read a complete line from stream if available otherwise leave all bytes on
 * the stream.
 */
ssize_t nonblock_getline(char *lineptr, FILE *stream)
{
	ssize_t n = 0;
	ssize_t i = 0;
	int c;

	/*
	 * Read bytes off the stream until we reach the end of a line or EOF.
	 */
	do {
		c = fgetc(stream);
		if (c != EOF) {
			lineptr[n] = c;
			n++;
		}
	} while (c != '\n' && c != EOF);

	lineptr[n] = '\0';

	/*
	 * If we do not have a complete line then push all the read characters
	 * back into the stream.
	 */
	if (c == EOF) {
		for (i = n - 1; i >= 0; i--)
			ungetc(lineptr[i], stream);

		return -1;
	}

	return n;
}
