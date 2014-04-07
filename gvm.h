#include <stdbool.h>

#include "point.h"


enum coordmode {
	MODE_NONE,
	MODE_ABSOLUTE,
	MODE_RELATIVE
};


struct command {
	char prefix;
	unsigned int code;
};


struct gvm {
	/* config */
	bool verbose;
	bool sloppy;
	bool unlocated_moves;

	/* state */
	FILE *gcode;
	unsigned int counter;

	enum coordmode mode;
	bool located;

	struct point position;
	struct point previous;
	struct point offset;
	struct point prevoffset;
};


void gcerr(const char *msg);

bool feol(FILE *stream);
void fconsume(FILE *stream, char stop);

bool check_line_discard(FILE *stream, char c);

void gvm_init(struct gvm *m, bool verbose);
void gvm_load(struct gvm *m, const char *path);
void gvm_close(struct gvm *m);

int gvm_read(struct gvm *m, struct command *cmd, struct point *result,
							enum axismask *mask);

void gvm_apply(struct gvm *m, struct command *cmd, struct point *values,
							enum axismask *mask);

int gvm_step(struct gvm *m);
void gvm_run(struct gvm *m);

unsigned int gvm_get_counter(struct gvm *m);
int gvm_get_position(struct gvm *m, struct point *result, bool physical);
int gvm_get_delta(struct gvm *m, struct point *result, bool physical);
