#define SERIAL_VMIN	0
#define SERIAL_VTIME	1


int serial_init(const char* serialport, int baud);
int serial_getline(int serial, char *buffer, int timeout);
