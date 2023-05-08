#include "parser.h"
#include "vec.h"
#include "vecstring.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void critical_err(const char* str)
{
	fprintf(stderr, "Critical error: %s\n", str);
	exit(1);
}

static const char* skip_ws(const char* in)
{
	for (; isspace(*in); ++in)
		;
	return in;
}

enum stop_reason {
	END_OF_LINE,
	WHITESPACE
};

static int push_word(string* str, const char** line)
{
	int state_dq = 0;
	const char* tmp = *line;
	for (char c; (c = *tmp++) != '\0';) {
		if (c == '"') {
			state_dq = !state_dq;
			continue;
		}
		if (state_dq == 0 && isspace(c)) {
			*line = tmp;
			return WHITESPACE;
		} else {
			string_push(str, c);
		}
	}
	*line = tmp;
	return END_OF_LINE;
}

VEC_DECLARE(vec_string, char*);

VEC_DECLARE(vec_cmds, shell_cmd);

int parse_line(parser_result* res, const char* line)
{
	vec_string out = vec_string_init();
	int argc = 0;
	for (;;) {
		line = skip_ws(line);
		string str = string_init();
		int res = push_word(&str, &line);
		vec_string_push(&out, &str.buf);
		argc++;
		if (res == WHITESPACE) {
			continue;
		} else if (res == END_OF_LINE) {
			break;
		}
	}
	const int cmdcount = 1;
	shell_cmd* cmds = malloc(sizeof(shell_cmd) * cmdcount);
	for (int i = 0; i != cmdcount; ++i) {
		cmds[i] = (shell_cmd) {
			.stdinid = STDIN_FILENO,
			.stdoutid = STDOUT_FILENO,
			.stderrid = STDERR_FILENO,
			.argc = argc,
			.argv = out.buf
		};
	}

	res->is_async = 0;
	res->cmdlist.size = cmdcount;
	res->cmdlist.commands = cmds;
	res->stdinfile = NULL;
	return 0;
}

void parser_result_dealloc(parser_result* in)
{
	for (int i = 0; i != in->cmdlist.size; ++i) {
		for (int j = 0; j != in->cmdlist.commands->argc; ++j) {
			free(in->cmdlist.commands->argv[j]);
		}
		free(in->cmdlist.commands[i].argv);
	}
	free(in->cmdlist.commands);
}
