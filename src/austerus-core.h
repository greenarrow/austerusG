#define MSG_ACK			"ok"
#define MSG_ACK_LEN		2
#define MSG_DUD			"rs 0 Dud"
#define MSG_DUD_LEN		8

#define SERIAL_LINE_LEN		255
#define SERIAL_INIT_PAUSE	500000
#define SERIAL_RETRIES		4

#define DEFAULT_BAUDRATE	57600
#define DEFAULT_ACK_COUNT	1


void usage(void);
int main(int argc, char* argv[]);
void leave(int signal);
