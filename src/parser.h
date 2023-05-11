#ifndef PARSER_H
#define PARSER_H

// wartosci atrybutow do obslugi plikow
typedef enum cmd_attributes {
	ATTRIBUTE_NONE   = 0,
	ATTRIBUTE_APPEND = 1,
	ATTRIBUTE_TRUNC  = 2,
	ATTRIBUTE_EXCL   = 4,
	ATTRIBUTE_PIPE   = 8,
	ATTRIBUTE_STDOUT = 16,
	ATTRIBUTE_STDERR = 32,
	ATTRIBUTE_STDIN  = 32
} cmd_attributes;

// pojedyncza komenda
typedef struct shell_cmd {
	int argc;
	char** argv;
} shell_cmd;

// lista komend
typedef struct cmd_list {
	shell_cmd* commands;
	int size;
} cmd_list;

// Przetworzona wczytana linia, podzielona na odpowiednio komendy i ich
// argumenty, pliki do wejscia oraz wyjscia + flaga async
typedef struct parser_result {
	cmd_list cmdlist;
	char* stdinfile;
	char* stdoutfile;
	cmd_attributes attrib;
	int is_async;
} parser_result;

int parse_line(parser_result* res, const char* line);
void parser_result_dealloc(parser_result* res);

#endif
