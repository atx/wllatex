
// TODO: Link with external libargp for musl etc
#include <argp.h>
#include <stdlib.h>

#include "args.h"


const char *argp_program_version = "wllatex " VERSION;
const char *argp_program_bug_address = "Josef Gajdusek <atx@atx.name>";


static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct args *args = state->input;
#define FAIL(with, ...) argp_failure(state, EXIT_FAILURE, 0, with, ##__VA_ARGS__);
	switch (key) {
	case 'v':
		args->verbose = true;
		break;
	case ARGP_KEY_END:
		break;
	}
#undef FAIL

	return 0;
}


static struct argp_option argp_options[] = {
	{ "verbose",	'v',		NULL,			0,		"Enable verbose logging",	0 },
	{ NULL, 0, NULL, 0, NULL, 0 }
};


static const struct argp argp_parser = {
	.options = argp_options,
	.parser = parse_opt,
	.args_doc = NULL,
	.doc = "LaTeX IME for wayland",
	.children = NULL,
	.help_filter = NULL,
	.argp_domain = NULL
};


int parse_args(struct args *args, int argc, char *argv[])
{
	args->verbose = false;
	error_t err = argp_parse(&argp_parser, argc, argv, ARGP_NO_EXIT, NULL, args);
	return err;
}
