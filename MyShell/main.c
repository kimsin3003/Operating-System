#include "header.h"

int main(){

	char cmdline[MAXLINE];

	while (1){
		printf("%s", prompt);
		fflush(stdout);

		if (fgets(cmdline, MAXLINE, stdin) == NULL)
			return 1;

		process_cmd(cmdline);
		fflush(stdout);

		printf("\n");
	}

	return 0;
}

void process_cmd(char *cmdline)
{
	int argc;
	char *argv[MAXARGS];

    argc = parse_line(cmdline, argv);


    if (builtin_cmd(argc, argv) == 0) {
		return;
	}
    
    return;
}


int parse_line(char *cmdline, char **argv)
{
	int argc = 0;
	argv[argc] = strtok(cmdline, " \t\n");

	while (argv[argc] != NULL){
		argc++;
		argv[argc] = strtok(NULL, " \t\n");
		
	}


	return argc;
}


int builtin_cmd(int argc, char **argv)
{

    if ((!strcmp(argv[0], "quit") || (!strcmp(argv[0], "exit")))) {
		exit(0);
	}

	printf("argument count: %d\n", argc);
	printf("arguments:");
	for (int i = 0; i < argc; i++) {
		printf(" %s", argv[i]);
	}

	return 1;
}

