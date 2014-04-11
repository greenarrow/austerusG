#include <stdbool.h>
#include "point.h"


struct peaks {
	long int min;
	long int max;
};


struct extends {
	struct peaks x;
	struct peaks y;
	struct peaks z;
	struct peaks e;
};


struct region {
	long int x1;
	long int x2;
	long int y1;
	long int y2;
};


void bounds_clear(struct extends *value);

float get_progress_table(unsigned int **table, size_t *lines,
                                                        const char *filename);
size_t get_extends(struct extends *bounds, bool deposition,
	bool physical, bool zmode, long int zmin, struct region *ignore,
	bool verbose, const char *filename);
