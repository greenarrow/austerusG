#include <stdbool.h>
#include "point.h"


struct peaks {
	float min;
	float max;
};


struct extends {
	struct peaks x;
	struct peaks y;
	struct peaks z;
	struct peaks e;
};


struct region {
	float x1;
	float x2;
	float y1;
	float y2;
};


void bounds_clear(struct extends *value);

float get_progress_table(unsigned int **table, size_t *lines,
                                                        const char *filename);
size_t get_extends(struct extends *bounds, bool deposition,
	bool physical, bool zmode, float zmin, struct region *ignore,
	bool verbose, const char *filename);
