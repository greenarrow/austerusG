#define BAR_WIDTH		35
#define PIPE_LINE_BUFFER_LEN	100


void print_time(int seconds);
void print_status(int pct, int taken, int estimate);
void print_file(FILE *stream_gcode, FILE *stream_input, int verbose);
int main();