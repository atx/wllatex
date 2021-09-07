
#pragma once

#include <stdbool.h>

struct args {
	bool verbose;
	bool oneshot;
};

int parse_args(struct args *args, int argc, char *argv[]);
