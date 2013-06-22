#define SERIAL_LINE_LEN		2048
#define SERIAL_INIT_PAUSE	500000


void usage(void);
int main(int argc, char* argv[]);
void process_command(char *line);
void leave(int signal);
