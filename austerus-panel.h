#define KEY_ESC 27

/*
 * TODO why can't we read when this is one?
 * premature readlines seem to loose us data!
 */
#define TEMP_PERIOD	2
#define EXTRUDE_PERIOD	0.5

#define VERSION	"0.1"

#define BIT_H	0x1
#define BIT_PZ	0x2
#define BIT_END	0x4
#define BIT_MZ	0x8
#define BIT_PY	0x10
#define BIT_MX	0x20
#define BIT_MY	0x40
#define BIT_PX	0x80

#define BIT_E	0x100
#define BIT_R	0x200
#define BIT_D	0x400


#define DEFAULT_JOG_DISTANCE	20
#define DEFAULT_JOG_SPEED	500
#define DEFAULT_TEMP_TARGET	250
#define DEFAULT_FEEDRATE	100

/* This value is important as it impacts how frequently the extruder moves */
#define CURSES_TIMEOUT		100

#define LINE_FEEDBACK_LEN	256

#define PANEL_POS_KEYS_X	50
#define PANEL_POS_KEYS_Y	1
#define PANEL_POS_NUMBERS_X	28
#define PANEL_POS_NUMBERS_Y	1
#define PANEL_POS_EXTRUDER_X	39
#define PANEL_POS_EXTRUDER_Y	1

void print_box(int x, int y, int w, int h, char *label, int bold);
void print_keys(int x, int y, int mask);
void print_fkeys(int x, int y);
void print_temperature(unsigned int temp_current, unsigned int temp_target);
void print_feedrate(unsigned int feedrate);
void print_jog_distance(unsigned int jog_distance);
void print_jog_speed(unsigned int jog_speed);
void print_fan(unsigned int mode);

int home_axis(FILE *streamGcode, char axis);
int end_axis(FILE *streamGcode, char axis);

int main(int argc, char* argv[]);


