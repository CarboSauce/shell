#include "parser.h"
#include "vec.h"
#include "vecstring.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
// ominieci bialych znakow w linii
static const char* skip_ws(const char* in)
{
	for (char c; (c = *in) != '\0'; ++in) {
		if (!isspace(c))
			break;
	}
	return in;
}

static int push_word(string* str, const char** line)
{
	int state_dq    = 0;
	int state_bcksl = 0;
	const char* tmp = *line;
	if (*tmp == '\0')
		return END_OF_LINE;
	for (char c; (c = *tmp) != '\0'; ++tmp) {
		if (state_bcksl != 1) {
			if (c == '\\' && state_dq == 0) {
				state_bcksl = 1;
				continue;
			} else if (c == '"') {
				state_dq = !state_dq;
				continue;
			} else if (c == '#') {
				*line = tmp;
				return END_OF_LINE;
			} 
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
// ustawianie flag w zaleznosci od wczytanych symboli
static cmd_attributes parse_symbol(const char** in)
{
	const char* tmp           = *in;
	enum cmd_attributes flags = ATTRIBUTE_NONE;
	int moveahead             = 0;
	switch (*tmp) {
	case '>': {
		flags |= ATTRIBUTE_STDOUT;
		moveahead++;
		if (tmp[1] == '>') {
			moveahead++;
			flags |= ATTRIBUTE_APPEND;
		} else if (tmp[1] == '|') {
			moveahead++;
			flags |= ATTRIBUTE_TRUNC;
		} else {
			flags |= ATTRIBUTE_EXCL;
		}
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
	*in = skip_ws(tmp + moveahead);
	return flags;
}

void cleanup_vecs(vec_cmds* in1, vec_string* in2, string* in3)
{
	string_deinit(in3);
	vec_string_deinit(in2);
	vec_cmds_deinit(in1);
}
// glowna funkcja do przetworzenia linii
int parse_line(parser_result* res, const char* line)
{
	vec_string out = vec_string_init();
	vec_cmds cmds  = vec_cmds_init();
	int isasync    = 0;
	int redirstate = 0;
	int attribs    = 0;
	char* stdinf   = NULL;
	char* stdoutf  = NULL;
	for (;;) {
		line = skip_ws(line);
		if (line[0] == '&') {
			line = skip_ws(line + 1);
			if (line[0] != '\0') {
				fprintf(stderr, "%s: Expected nothing after &\n", progname);
				vec_string_deinit(&out);
				vec_cmds_deinit(&cmds);
				return 1;
			}
			isasync = 1;
			break;
		}
		cmd_attributes redi = parse_symbol(&line);

		if (redi != ATTRIBUTE_NONE && redi != ATTRIBUTE_STDIN
			&& (redi & ATTRIBUTE_STDOUT) == 0) {
			char* argv_needs_null = NULL;
			vec_string_push(&out, &argv_needs_null);
			shell_cmd tmp
				= (shell_cmd) { .argc = out.size - 1, .argv = out.buf };
			vec_cmds_push(&cmds, &tmp);
			out = vec_string_init();
		}

		string str = string_init();
		int res    = push_word(&str, &line);

		if (res == END_OF_LINE && out.size == 0 && cmds.size == 0
			&& str.size == 0) {
			cleanup_vecs(&cmds, &out, &str);
			return 1;
		}
		if (redirstate == 1 && redi == ATTRIBUTE_NONE && res != END_OF_LINE) {
			if (redi == ATTRIBUTE_PIPE) {
				fprintf(stderr,
					"%s: Unexpected pipe after stdout redirection\n",
					progname);
			} else {
				fprintf(stderr,
					"%s: Only symbols expected after stdout redirection\n",
					progname);
			}
			cleanup_vecs(&cmds, &out, &str);
			return 1;

		} else if (redi == ATTRIBUTE_STDIN) {
			attribs |= ATTRIBUTE_STDIN;
			if (str.size == 0) {
				fprintf(
					stderr, "%s: Missing file to redirect stdin\n", progname);
				cleanup_vecs(&cmds, &out, &str);
				return 1;
			}
			stdinf = str.buf;
			line   = skip_ws(line);
			if (line[0] == '&') {
				line = skip_ws(line + 1);
				if (line[0] != '\0') {
					fprintf(stderr, "%s: Expected nothing after &\n", progname);
					cleanup_vecs(&cmds, &out, &str);
					return 1;
				}
				isasync = 1;
				res     = END_OF_LINE;
			}
		} else if ((redi & ATTRIBUTE_STDOUT) != 0) {
			if (str.size == 0) {
				fprintf(
					stderr, "%s: Missing file to redirect stdin\n", progname);
				cleanup_vecs(&cmds, &out, &str);
				return 1;
			}
			attribs |= redi;
			stdoutf    = str.buf;
			redirstate = 1;
		} else if (str.size != 0)
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
	vec_string_push(&out, &argv_needs_null);
	shell_cmd tmp = (shell_cmd) {
		.argc = out.size - 1,
		.argv = out.buf,
	};
	vec_cmds_push(&cmds, &tmp);
	res->is_async         = isasync;
	res->cmdlist.size     = cmds.size;
	res->cmdlist.commands = cmds.buf;
	res->stdinfile        = stdinf;
	res->stdoutfile       = stdoutf;
	res->attrib           = attribs;
	return 0;
}
// dealokacja pamieci wyniku parsowania
void parser_result_dealloc(parser_result* in)
{
	for (int i = 0; i != in->cmdlist.size; ++i) {
		for (int j = 0; j != in->cmdlist.commands[i].argc; ++j) {
			free(in->cmdlist.commands[i].argv[j]);
		}
		free(in->cmdlist.commands[i].argv);
	}
	free(in->cmdlist.commands);
	free(in->stdinfile);
	free(in->stdoutfile);
}
