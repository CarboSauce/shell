#include "parser.h"
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <pwd.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>










typedef struct process_ctx {
	int stdin_fd;
	int stdout_fd;
	int status;
} process_ctx;


typedef struct process_list {
	int pipes_len;
	int (*pipes)[2];
	process_ctx* processes;
} process_list;


//to do przerobienia bo chuj
void p_close(process_list* p_list){
	for (int i = 0; i < p_list->pipes_len; i++) {
		close(p_list->pipes[i][0]);
		close(p_list->pipes[i][1]);
	}
}



void execute(cmd_list* command_list, process_list* p_list, int current)
{
	if(current == 0)
	{
	dup2(p_list->processes[current].stdout_fd,STDOUT_FILENO);
	}

	//if (cmdlist->commands.attrib & ATTRIBUTE_PIPE != 0)
	//{
//	
//	}
	else if(current == p_list->pipes_len)
	{
	dup2(p_list->processes[current].stdin_fd, STDIN_FILENO);
	}
	else{
	dup2(p_list->processes[current].stdout_fd,STDOUT_FILENO);
	dup2(p_list->processes[current].stdout_fd,STDOUT_FILENO);
	}
	p_close(p_list);
	return execvp(command_list->commands->argv[0],command_list->commands->argv);
	//saluting emoji, oby dzialalo bo mnie popierdoli do reszty

}

pid_t run(cmd_list* commandlist, process_list p_list, int current)
{
	pid_t child_pid = fork();
	if(child_pid<0) 
	//tutaj chuj
	{}
	else if(child_pid){
	return child_pid;
	} else{
	execute(commandlist, &p_list, current);
	return 0;
	}
}

bool pajpik(parser_result* in)
{
	process_list p_list;
	p_list.pipes_len=in->cmdlist.size-1;
	p_list.pipes = calloc(sizeof(int[2]), p_list.pipes_len);

	p_list.processes = malloc(sizeof(process_ctx) * in->cmdlist.size);
//z tymi ++i moze sie yebac, trzeba bd sprawdzic jak bd siadalo cos
	for(int i=1; i<p_list.pipes_len;++i)
	{
		pipe(p_list.pipes[i-1]);
		p_list.processes[i].stdin_fd = p_list.pipes[i-1][0];
		p_list.processes[i-1].stdout_fd = p_list.pipes[i-1][1];

	}
	
	for (int i =0; i<p_list.pipes_len+1;++i)
	{
		run(&in->cmdlist.commands[i], p_list, i);
	}
	//tu jeszcze jest ten close i for duzo wait null
	p_close(&p_list);

	for(int i=0; i< p_list.pipes_len+1;++i){
		wait(NULL);
	}
	return 0;
}




































char curdir[PATH_MAX];
const char* progname;

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
		printf("Cmdlist(%d): \n", i);
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

	return BUILTIN_NONE;
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
	case BUILTIN_EXPORT:
	case BUILTIN_HISTORY:
		printf("Unimplemented builtin: %s", cmd->argv[0]);
	case BUILTIN_EXIT:
	case BUILTIN_NONE:
		break;
	}
}

int main(int argc, char** argv)
{
	(void)argc;
	progname = argv[0];

	set_cwd();

	char* prompt;
	if (prompt_init(&prompt) < 0) {
		perror(argv[0]);
		return 1;
	}

	read_history(NULL);
	bool running = true;
	while (running) {
		printf(prompt, curdir);
		char* buf = readline("$ ");
		if (buf == NULL)
			continue;
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
		pajpik(&pars);
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
