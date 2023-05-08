#include "parser.h"
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <pwd.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>

char curdir[PATH_MAX];
const char* progname;
char* prompt;

void logerr()
{
	perror(progname);
	exit(1);
}

int prompt_init(char** prompt)
{
	char hname[HOST_NAME_MAX + 1];
	if (gethostname(hname, sizeof hname) == -1)
		return -1;
	const char* login = getlogin();
	if (login == NULL)
		return -2;
	const char* fmt = "%s@%s %%s\n";
	int sz = snprintf(NULL, 0, fmt, login, hname);
	char* buf = malloc(sz + 1);
	if (buf == NULL)
		return -3;
	snprintf(buf, sz + 1, fmt, hname, login);
	*prompt = buf;
	return 0;
}

void print_cmdline(parser_result* in)
{
	for (int i = 0; i != in->cmdlist.size; ++i) {
		printf("Cmdlist(%d); Attribute(%d) stdinf(%s) isasync(%d)\n"
				, i
				, in->cmdlist.commands[i].attrib
				, in->stdinfile
				, in->is_async);
		for (int j = 0; j != in->cmdlist.commands[i].argc; ++j) {
			printf("\targ(%d): %s\n", j, in->cmdlist.commands[i].argv[j]);
		}
	}
}

enum builtin {
	BUILTIN_CD,
	BUILTIN_EXIT,
	BUILTIN_HISTORY,
	BUILTIN_ECHO,
	BUILTIN_EXPORT,
	BUILTIN_UNEXPORT,
	BUILTIN_NONE,
};

void set_cwd()
{
	if (getcwd(curdir, sizeof curdir) == NULL) {
		logerr();
	}
}

enum builtin detect_builtin(shell_cmd* in)
{
	if (strcmp(in->argv[0], "cd") == 0)
		return BUILTIN_CD;
	if (strcmp(in->argv[0], "exit") == 0)
		return BUILTIN_EXIT;
	if (strcmp(in->argv[0], "history") == 0)
		return BUILTIN_HISTORY;
	if (strcmp(in->argv[0], "echo") == 0)
		return BUILTIN_ECHO;
	if (strcmp(in->argv[0], "export") == 0)
		return BUILTIN_EXPORT;
	if (strcmp(in->argv[0], "unexport") == 0)
		return BUILTIN_UNEXPORT;

	return BUILTIN_NONE;
}

void print_history()
{
	HIST_ENTRY** his = history_list();
	if (his == NULL)
		return;
	HIST_ENTRY* entry;
	for (int i = 0; (entry = *his++) != NULL; i++) {
		printf("%d\t%s\n", i, entry->line);
	}
}

void handle_builtin(const shell_cmd* cmd, enum builtin in)
{
	switch (in) {
	case BUILTIN_CD:
		if (cmd->argc != 2) {
			printf("cd: Expected single argument\n");
			break;
		}
		if (chdir(cmd->argv[1]) == 0)
			set_cwd();
		else
			fprintf(stderr, "cd: %s: %s\n", cmd->argv[1], strerror(errno));
		break;
	case BUILTIN_ECHO:
		for (int i = 1; i != cmd->argc; ++i) {
			fputs(cmd->argv[i], stdout);
			if (i != cmd->argc - 1)
				putchar(' ');
		}
		putchar('\n');
		break;
	case BUILTIN_HISTORY:
		print_history();
		break;
	case BUILTIN_EXPORT: {
		if (cmd->argc < 3) {
			printf("export: Expected at least 2 arguments\n");
			break;
		}
		int overwrite = 0;
		const char *in, *out;
		if (strcmp(cmd->argv[1], "-o") == 0) {
			if (cmd->argc != 4) {
				printf("export: Expected 2 arguments");
				break;
			}
			overwrite = 1;
			in = cmd->argv[2];
			out = cmd->argv[3];
		} else {
			in = cmd->argv[1];
			out = cmd->argv[2];
		}
		if (setenv(in, out, overwrite) == -1) {
			perror("export");
		}
	} break;
	case BUILTIN_UNEXPORT:
		if (cmd->argc != 2) {
			printf("unexport: Expected 1 argument");
			break;
		}
		if (unsetenv(cmd->argv[1]) == -1) {
			perror("unexport");
		}
		break;
	case BUILTIN_EXIT:
	case BUILTIN_NONE:
		break;
	}
}


atomic_int sigint_var,sigquit_var,sigchld_var;

void sig_handler(int id) {
	switch(id) {
		case SIGQUIT:
			sigquit_var = 1;
		break;
		case SIGINT:
			sigint_var = 1;
		break;
		case SIGCHLD:
			sigchld_var++;
		break;
	}
}

void sigint_handler(int id) {
	rl_free_line_state();
	rl_cleanup_after_signal();
	RL_UNSETSTATE(RL_STATE_ISEARCH
		| RL_STATE_ISEARCH
		| RL_STATE_NSEARCH
		| RL_STATE_VIMOTION
		| RL_STATE_NUMERICARG
		| RL_STATE_MULTIKEY);
	rl_point = rl_end = rl_mark = 0;
	rl_line_buffer[0] = 0;
	write(STDOUT_FILENO,"\n",1);
}

int signal_hook() {
	int reset = 0;
	if (sigint_var) {
		sigint_var = 0;
		reset = 1;
	}
	if (sigquit_var) {
		sigquit_var = 0;
		reset = 1;
		putchar('\n');
		print_history();
	}
	if (reset) {
	rl_cleanup_after_signal();
	putchar('\n');
	rl_on_new_line();
	rl_replace_line("",0);
	rl_redisplay();
	//rl_on_new_line();
	//RL_UNSETSTATE(RL_STATE_ISEARCH
	//	| RL_STATE_ISEARCH
	//	| RL_STATE_NSEARCH
	//	| RL_STATE_VIMOTION
	//	| RL_STATE_NUMERICARG
	//	| RL_STATE_MULTIKEY);
	//rl_point = rl_end = rl_mark = 0;
	//rl_line_buffer[0] = 0;
	}
	return 0;
}

int main(int argc, char** argv)
{
	(void)argc;
	progname = argv[0];

	set_cwd();

	if (prompt_init(&prompt) < 0) {
		perror(argv[0]);
		return 1;
	}
	//rl_clear_signals();
	signal(SIGINT,sig_handler);
	signal(SIGQUIT,sig_handler);
	signal(SIGCHLD,sig_handler);
	rl_signal_event_hook = signal_hook;

	read_history(NULL);
	

	bool running = true;
	while (running) {
		printf(prompt, curdir);
		char* buf = readline("$ ");
		if (buf == NULL) {
			printf("readline returned null\n");
			exit(1); // temporary
			continue;
		}
		parser_result pars;
		int res = parse_line(&pars, buf);
		if (res) {
			puts("NO COMMAND");
			goto rl_cleanup;
		}
		enum builtin tmp = detect_builtin(pars.cmdlist.commands);
		if (tmp == BUILTIN_EXIT)
			running = false;
		else if (tmp == BUILTIN_NONE)
			printf("TODO: run command\n");
		else
			handle_builtin(pars.cmdlist.commands, tmp);

		print_cmdline(&pars);
		parser_result_dealloc(&pars);
		add_history(buf);
	rl_cleanup:
		free(buf);
	}
	// dealloc resources
	write_history(NULL);
	clear_history();
	free(prompt);
}
