#include <linux/limits.h>
#include <stdio.h>
#include <readline/readline.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>

int prompt_init(char** prompt) {
	char hname[HOST_NAME_MAX+1];
	if (gethostname(hname,sizeof hname) == -1)
		return -1;
	const char* login = getlogin();
	if (login == NULL)
		return -2;
	const char* fmt = "%s@%s %%s\n";
	int sz = snprintf(NULL,0,fmt,hname,login);
	char* buf = malloc(sz+1);
	if (buf == NULL)
		return -3;
	snprintf(buf,sz+1,fmt,hname,login);
	*prompt = buf;
	return 0;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	char wdir[PATH_MAX];
	int result;
	if (getcwd(wdir,sizeof wdir) == NULL) {
		perror(argv[0]);
		return 1;
	}
	char* prompt;
	if ((result = prompt_init(&prompt) ) < 0) {
		perror(argv[0]);
		return 1;
	}
	int run = 1;
	while(run) {
		printf(prompt,wdir);
		char* buf = readline("> ");
		printf("Line input: (%s)\n",buf);
		if (strcmp(buf,"exit") == 0)
			run = 0;
		free(buf);
	}

	free(prompt);

}
