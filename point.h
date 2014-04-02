#ifndef H_POINT
#define H_POINT

enum axismask {
	AXIS_NONE = 0,
	AXIS_X = 1,
	AXIS_Y = 2,
	AXIS_Z = 4,
	AXIS_E = 8,
	AXIS_ALL = 255
};


struct point {
	float x;
	float y;
	float z;
	float e;
};


void point_clear(struct point *value, enum axismask *mask);
void point_cpy(struct point *dst, struct point *src, enum axismask *mask);
void point_delta(struct point *value, struct point *delta, enum axismask *mask,
								int direction);
void point_print(FILE *stream, struct point *value);

#endif
