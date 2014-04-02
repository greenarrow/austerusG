#include <stdio.h>
#include <stdlib.h>


/*
 *
 */
void bail(const char *prefix)
{
	perror(prefix);
	exit(2);
}
