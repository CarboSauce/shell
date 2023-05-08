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

extern const char* progname;

enum stop_reason {
	END_OF_LINE,
	WHITESPACE,
	NO_MATCH,
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
	enum stop_reason reason;
	enum cmd_attributes attr;
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
	int state_bcksl = 0;
	const char* tmp = *line;
	if (*tmp == '\0')
		return END_OF_LINE;
	for (char c; (c = *tmp++) != '\0';) {
		if (c == '\\' && state_bcksl != 1) {
			state_bcksl = 1;
			continue;
		} else if (c == '"' && state_bcksl != 1) {
			state_dq = !state_dq;
			continue;
		}
		if (state_bcksl == 1) {
			string_push(str, c);
			state_bcksl = 0;
		} else if (state_dq == 0 && isspace(c)) {
			*line = tmp;
			return WHITESPACE;
		} else {
			string_push(str, c);
		}
	}
	*line = tmp;
	return END_OF_LINE;
}



static cmd_attributes parse_symbol(const char** in)
{
	const char* tmp = *in;
	enum stop_reason symbol;
	enum cmd_attributes flags = ATTRIBUTE_NONE; 
	int moveahead = 0;
	switch (*tmp) {
	case '1': 
		if (*tmp == '>') {
			flags |= ATTRIBUTE_STDOUT | ATTRIBUTE_CREAT;
			moveahead++;
			
			if (tmp[2] == '>') {
				moveahead++;
				flags |= ATTRIBUTE_APPEND;
			} else if (tmp[2] == '|') {
				moveahead++;
				flags |= ATTRIBUTE_TRUNC;
			}
		} else {
			flags = ATTRIBUTE_NONE;
		}
		break;
	case '2':
		if (tmp[1] == '>') {
			flags |= ATTRIBUTE_STDOUT | ATTRIBUTE_CREAT;
			moveahead+=2;
			
			if (tmp[2] == '>') {
				moveahead++;
				flags |= ATTRIBUTE_APPEND;
			} else if (tmp[2] == '|') {
				moveahead++;
				flags |= ATTRIBUTE_TRUNC;
			};
		} else {
			flags = ATTRIBUTE_NONE;
		} 
		break;
	case '>': {
		flags |= ATTRIBUTE_STDOUT | ATTRIBUTE_CREAT;
		moveahead++;
		if (tmp[1] == '>') {
			moveahead++;
			flags |= ATTRIBUTE_APPEND;
		} else if (tmp[1] == '|') {
			moveahead++;
			flags |= ATTRIBUTE_TRUNC;
		};
		break;
	}
	case '|':
		flags |= ATTRIBUTE_PIPE;
		moveahead++;
		break;
	case '<':
		flags |= ATTRIBUTE_STDIN;
		moveahead++;
		break;
	default:
		flags |= ATTRIBUTE_NONE;
		break;
	}
	*in = skip_ws(tmp+moveahead);
	return flags; 
}

void cleanup_vecs(vec_cmds* in1, vec_string* in2, string* in3) {
	vec_cmds_deinit(in1);
	vec_string_deinit(in2);
	string_deinit(in3);
}

int parse_line(parser_result* res, const char* line)
{
	vec_string out = vec_string_init();
	vec_cmds cmds = vec_cmds_init();
	const char* stdinf;
	for (;;) {
		line = skip_ws(line);
		cmd_attributes redi = parse_symbol(&line);
		if (redi != ATTRIBUTE_NONE) {
		 	char* argv_needs_null = NULL;
		 	vec_string_push(&out,&argv_needs_null);
		 	shell_cmd tmp = (shell_cmd) {
		 		.attrib = redi,
		 		.argc = out.size,
		 		.argv = out.buf
		 	};
		 	vec_cmds_push(&cmds, &tmp);
		 	out = vec_string_init();
		}
		
		string str = string_init();
		int res = push_word(&str, &line);

		if (res == END_OF_LINE 
		 && out.size == 0
		 && cmds.size == 0
		 && str.size == 0) {
			cleanup_vecs(&cmds, &out, &str);
			return 1;
		}

		if (redi == ATTRIBUTE_STDIN) {
			if (str.size == 0) {
				fprintf(stderr,"%s: Missing file to redirect stdin\n",progname);
				return 1;
			}
			stdinf = str.buf;
		}
		if (str.size != 0)
			vec_string_push(&out, &str.buf);

		if (res == WHITESPACE) {
			continue;
		} else if (res == END_OF_LINE) {
			if (str.size == 0)
				string_deinit(&str);
			break;
		}
	}

	char* argv_needs_null = NULL;
	vec_string_push(&out,&argv_needs_null);
	shell_cmd tmp = (shell_cmd) {
		.attrib = ATTRIBUTE_NONE,
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
