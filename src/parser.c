#include "parser.h"
#include "vec.h"
#include "vecstring.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

VEC_DECLARE(vec_string, char*);

VEC_DECLARE(vec_cmds, shell_cmd);

enum stop_reason {
	END_OF_LINE,
	WHITESPACE,
	EOL_FRONT
};

enum symbol {
	SYMBOL_STDIN,
	SYMBOL_STDOUT,
	SYMBOL_STDERR,
	SYMBOL_PIPE,
	SYMBOL_EOL,
	SYMBOL_NOMATCH
};

typedef struct redirect {
	enum symbol symbol;
	int openflags;
} redirect;

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
static int push_word(string* str, const char** line)
{
	int state_dq = 0;
	const char* tmp = *line;
	if (*tmp == '\0')
		return EOL_FRONT;
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



static redirect parse_symbol(const char** in)
{
	const char* tmp = *in;
	enum symbol symbol;
	int flags = 0;
	int moveahead = 0;
	switch (*tmp) {
	case '1': 
		if (*tmp == '>') {
			flags = O_CREAT;
			symbol = SYMBOL_STDOUT;
			moveahead++;
			
			if (tmp[2] == '>') {
				moveahead++;
				flags |= O_APPEND;
			} else if (tmp[2] == '|') {
				moveahead++;
				flags |= O_TRUNC;
			}
		} else if (tmp[1] == '\0') {
			symbol = SYMBOL_EOL;
		} else {
			symbol = SYMBOL_NOMATCH;
		}
		break;
	case '2':
		if (tmp[1] == '>') {
			flags = O_CREAT;
			symbol = SYMBOL_STDOUT;
			moveahead+=2;
			
			if (tmp[2] == '>') {
				moveahead++;
				flags |= O_APPEND;
			} else if (tmp[2] == '|') {
				moveahead++;
				flags |= O_TRUNC;
			};
		} else if (tmp[1] == '\0') {
			symbol = SYMBOL_EOL;
		} else {
			symbol = SYMBOL_NOMATCH;
		}
		break;
	case '>': {
		flags = O_CREAT;
		symbol = SYMBOL_STDOUT;
		moveahead++;
		if (tmp[1] == '>') {
			moveahead++;
			flags |= O_APPEND;
		} else if (tmp[1] == '|') {
			moveahead++;
			flags |= O_TRUNC;
		};
		break;
	}
	case '|':
		symbol = SYMBOL_PIPE;
		moveahead++;
		break;
	case '<':
		symbol = SYMBOL_STDIN;
		moveahead++;
		break;
	case '\0':
		symbol = SYMBOL_EOL;
		break;
	default:
		symbol = SYMBOL_NOMATCH;
		break;
	}
	*in = skip_ws(tmp+moveahead);
	return (redirect) {
		.symbol = symbol,
		.openflags = flags
	};
}

int parse_line(parser_result* res, const char* line)
{
	vec_string out = vec_string_init();
	vec_cmds cmds = vec_cmds_init();
	for (;;) {
		line = skip_ws(line);
		redirect redi = parse_symbol(&line);
		if (redi.symbol != SYMBOL_NOMATCH
		 && redi.symbol != SYMBOL_EOL) {
		 	shell_cmd tmp = (shell_cmd) {
		 		.stdinid = 0,
		 		.stderrid = 0,
		 		.stdoutid = 0,
		 		.argc = out.size,
		 		.argv = out.buf
		 	};
		 	vec_cmds_push(&cmds, &tmp);
		 	out = vec_string_init();
		}
		
		string str = string_init();
		int res = push_word(&str, &line);

		if (res == EOL_FRONT && out.size == 0) {
			vec_cmds_deinit(&cmds);
			vec_string_deinit(&out);
			string_deinit(&str);
			return 1;
		}
		vec_string_push(&out, &str.buf);

		if (res == WHITESPACE) {
			continue;
		} else if (res == END_OF_LINE) {
			break;
		}
	}

	shell_cmd tmp = (shell_cmd) {
		.stdinid = 0,
		.stderrid = 0,
		.stdoutid = 0,
		.argc = out.size,
		.argv = out.buf
	};
	vec_cmds_push(&cmds, &tmp);
	res->is_async = 0;
	res->cmdlist.size = cmds.size;
	res->cmdlist.commands = cmds.buf;
	res->stdinfile = NULL;
	return 0;
}

void parser_result_dealloc(parser_result* in)
{
	for (int i = 0; i != in->cmdlist.size; ++i) {
		for (int j = 0; j != in->cmdlist.commands[i].argc; ++j) {
			free(in->cmdlist.commands[i].argv[j]);
		}
		free(in->cmdlist.commands[i].argv);
	}
	free(in->cmdlist.commands);
}
