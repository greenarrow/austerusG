#define MSG_ACK			"ok"

#define PIPE_LINE_BUFFER_LEN	100
#define SERIAL_LINE_BUFFER_LEN	255

#define SERIAL_INIT_PAUSE	500000
#define SERIAL_RETRIES		4
#define ACK_ENABLE		1

#define DEFAULT_BAUDRATE	57600
#define DEFAULT_ACK_COUNT	1


void usage(void);
int main(int argc, char* argv[]);
void leave(int signal);
