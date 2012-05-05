#define NONE        0
#define ABSOLUTE    1
#define RELATIVE    2

int read_axis(FILE *stream, char target, float *value);
void read_move(FILE *buffer, int mode, float *delta, float *position);
int read_extruded_delta(char *line, int *mode, float *delta, float *position);
float get_progress_table(unsigned int **table, size_t *lines, FILE *stream);
