#include <stdbool.h>


#define NONE        0
#define ABSOLUTE    1
#define RELATIVE    2


struct limit {
	float min;
	float max;
};


int axis_position(const char *axes, char axis);
int read_axis(FILE *stream, char target, float *value);
void read_move(FILE *buffer, int mode, char axis, float *delta,
	float *position);
int read_axis_delta(char *line, const char axis, int *mode, float *delta,
	float *position);
float get_progress_table(unsigned int **table, size_t *lines, FILE *stream);
size_t get_extends(struct limit *bounds, const char *axes, bool deposition,
	FILE *stream);

