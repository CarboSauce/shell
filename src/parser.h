#ifndef PARSER_H
#define PARSER_H

typedef struct shell_cmd {
	int stdinid;
	int stdoutid;
	int stderrid;
	int argc;
	char** argv;
} shell_cmd;

typedef struct cmd_list {
	shell_cmd* commands;
	int size;
} cmd_list;

typedef struct parser_result {
	cmd_list cmdlist;
	const char* stdinfile;
	int is_async : 1;
} parser_result;

int parse_line(parser_result* res, const char* line);
void parser_result_dealloc(parser_result* res);

#endif
