#define BAR_WIDTH		35
#define PIPE_LINE_BUFFER_LEN	100


void print_time(int seconds);
void print_status(int pct, int taken, int estimate);
ssize_t filter_comments(char *line);
void print_file(FILE *stream_gcode, FILE *stream_feedback, FILE *stream_input,
		size_t lines, unsigned int filament, unsigned int *table,
		int verbose);
int main();
