#define NORMAL          	0
#define STREAM			1
#define BAR_WIDTH		35
#define PIPE_LINE_BUFFER_LEN	100


void print_time(int seconds);
void print_status(int pct, int taken, int estimate);
ssize_t filter_comments(char *line);
int print_file(FILE *stream_input, size_t lines, const char *cmd,
	unsigned int filament, unsigned int *table, int mode, int verbose);
int main();
