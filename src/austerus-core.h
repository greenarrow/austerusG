#define MSG_ACK			"ok"
#define MSG_ACK_LEN		2
#define MSG_DUD			"rs 0 Dud"
#define MSG_DUD_LEN		8
#define MSG_CMD			"#ag:"
#define MSG_CMD_LEN		4

#define MSG_CMD_EXIT		"#ag:exit"
#define MSG_CMD_EXIT_LEN	8

#define SERIAL_LINE_LEN		255
#define SERIAL_INIT_PAUSE	500000

#define DEFAULT_BAUDRATE	57600
#define DEFAULT_TIMEOUT		10
#define DEFAULT_ACK_COUNT	1


void usage(void);
int main(int argc, char* argv[]);
void process_command(char *line);
void leave(int signal);
