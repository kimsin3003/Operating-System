#include "header.h"

int main(){
	int i;
	char cmdline[MAXLINE];
	int proc;
	int unExitedProc[100];
	int count = 0;
	
	while (1){
		printf("%s", prompt);
		fflush(stdout);

		if (fgets(cmdline, MAXLINE, stdin) == NULL)
			return 1;
			
				
		if((proc = process_cmd(cmdline)) > 0){
			unExitedProc[count] = proc;
			count++;
		}
		if(count > 0){
			if(waitpid(unExitedProc[count - 1], 0, WNOHANG)){
				unExitedProc[count - 1] = 0;
				count--;
			}
				
		}
		
		//reset variables.
		red_check = 0;
		bg_check = 0;
		pipe_check = 0;
		red_arg = NULL;
		for(i = 0; i < pipe_arg_count; i++){
			free(pipe_arg[i]);
		}
		pipe_arg_count = 0;
			
		fflush(stdout);
		printf("\n");
	}

	return 0;
}


int parse_line(char *cmdline, char **argv)
{
	
	int i, j;
	int argc = 0;
	argv[argc] = strtok(cmdline, delim);

	while (argv[argc] != NULL){
		argc++;
		argv[argc] = strtok(NULL, delim);
	}
	
	
	//redirection, pipe, background check 
	
	if(!strcmp(argv[argc - 1],"&")){//check background command
		argv[argc - 1] = NULL;
		argc--;
		bg_check = 1;
	}
	
	for(i = 0; i < argc; i++) {
		if(!strcmp(argv[i], ">")) {
			if(!argv[i+1]){
				printf("Not enough arguments\n");
				printf("Usage: [command] > [redirection dest]\n");
				return 0;
			}
			
			red_check = 1;
			red_arg = argv[i+1];
            argv[i] = NULL;
            argv[i+1] = NULL;
                
			argc = i;
			break;
		}
			
		
		else if(!strcmp(argv[i], "|")) {
			
			if(!argv[i+1]){
				printf("Not enough arguments\n");
				printf("Usage: [command] | [command]\n");
				return 0;
			}
			
			pipe_check = 1;
			for(j = 0; j < argc - i - 1; j++){ 
				pipe_arg[j] = (char*)malloc(sizeof(argv[i+j+1])*sizeof(char));
				strcpy(pipe_arg[j], argv[i+j+1]);
				argv[i+j+1] = NULL;
				pipe_arg_count++;
			}
			pipe_arg[pipe_arg_count] = NULL;
			argv[i] = NULL;
			argc = i;
			
			break;
		}
	}
	
	return argc;
}


int process_cmd(char *cmdline)
{
	int argc;
	char *argv[MAXARGS];
	int pid1, pid2, saved_stdout;
	int rfd;
	int pfd[2];
		
	if(!cmdline || cmdline[0] == '\n'){
		
		return 0;
	}
	
    argc = parse_line(cmdline, argv);
	
	if(red_check){
		rfd = open(red_arg, O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR | S_IWUSR);
		if(rfd == 0){
			printf("file open error\n");
			return 0;
		}
		saved_stdout = dup(1);
		dup2(rfd, 1);
		close(rfd);
	}
	
	if(!pipe_check && builtin_cmd(argc, argv) == 0) {//if command is built in command
	
		if(red_check){
			fflush(stdout);
			dup2(saved_stdout, 1);
			close(saved_stdout);
		}
		
		return 0;
	}
    
    //external commands and for reading pipe
    
	
	if(pipe_check){
		if(pipe(pfd) == -1){
			printf("pipe error\n");
			return 0;
		}
	}
	
	switch((pid1 = fork())){
	
	case -1:
		printf("fork 1 error\n");
		return 0;
		
	case 0://child process for output pipe and external command
		
		//pipe
		if(pipe_check){
			
			close(pfd[0]);
			dup2(pfd[1],1);
            
			if(builtin_cmd(argc,argv) == 0){
				exit(0);
			}
			else{
                execvp(argv[0], argv);
                printf("%s: Command not found\n", argv[0]);
			    exit(0);
			}
			
		}
		//external command
		else{
            execvp(argv[0], argv);
            printf("%s: Command not found\n", argv[0]);
            exit(0);			
        }
		
		break;
    default:
	//parent process
		
        if(bg_check){
            printf("[bg] %d : %s\n", pid1, argv[0]);

            if(waitpid(pid1, 0, WNOHANG) > 0)
                return 0;
            else
				return pid1;
        }

        else if(pipe_check){

            switch(pid2 = fork()){
            case -1: 
                printf("fork 2 error\n"); 
                return 0;

            case 0:
				close(pfd[1]);
                dup2(pfd[0], 0);
                //printf("pipecount: %d\n", pipe_arg_count);
                if(builtin_cmd(pipe_arg_count, pipe_arg) == 0){
                    exit(0);
                }
                else{
                    //printf("pipe in\n");
                    //printf("pipe %s, %s\n", pipe_arg[0], pipe_arg[1]);
                    execvp(pipe_arg[0], pipe_arg);
                    printf("%s: Command not found", pipe_arg[0]);// -> If exec got error, this line will be excuted.
                    exit(0);
                }
                break;

            default:
				close(pfd[0]);
				close(pfd[1]);
				while(wait(0)!=-1);
            }

        }
	
        else{
			if(red_check){
				fflush(stdout);
				dup2(saved_stdout, 1);
				close(saved_stdout);
			}
					
            waitpid(pid1, 0, 0);
		}
	}
	   
	return 0;
}


int builtin_cmd(int argc, char **argv)
{
	
	if (argc == 0) {
		printf("No command");
		return 1;
	}
	
    if ((!strcmp(argv[0], "quit") || (!strcmp(argv[0], "exit")))) {
		exit(0);
	}

	else if (!strcmp(argv[0], "ls")) {
		return list_files(argc, argv);
	}

	else if (!strcmp(argv[0], "ll")){
		return list_files_long(argc, argv);
	}
	
	else if (!strcmp(argv[0], "rm")){
		return remove_file(argc, argv);
	}
	
	else if (!strcmp(argv[0], "cp")){
		return copy_file(argc, argv);
	}
	
	else if (!strcmp(argv[0], "dcp")){
		return copy_directory(argc, argv);
	}
	
	else if (!strcmp(argv[0], "pwd")){
		return print_working_directory(argc);
	}
	
	else if (!strcmp(argv[0], "cd")){
		return change_directory(argc, argv);
	}

	else if (!strcmp(argv[0], "mv")){
		return move_file(argc, argv);	
	}
	
	else if (!strcmp(argv[0], "mkdir")){
		return make_directory(argc, argv);
	}
	
	else if (!strcmp(argv[0], "rmdir")){
		return remove_directory(argc, argv);
	}
	return 1;
}


int list_files(int argc, char **argv)
{
	DIR *dir_info;
	struct dirent *dir_entry;

	if(argc > 3){
		printf("Usage: ls");
		return 0;
	}
	
	if(argc == 2)
		dir_info = opendir(argv[1]);
		
	if(argc == 1)
		dir_info = opendir(".");
		
	if (NULL != dir_info)
	{
		while ((dir_entry = readdir(dir_info)))  
		{
			if((dir_entry->d_name)[0] == '.')
				continue;
			printf("%s\n", dir_entry->d_name);
		}
		closedir(dir_info);
	}
	
	else 
		printf("File Open Error");
		

	return 0;
}

int list_files_long(int argc, char **argv)
{
	DIR *dir_info;
	struct dirent  *dir_entry;
	struct stat file_info;
	struct passwd* user;
	char pathName[1024];
	
	if(argc > 2){
		printf("Usage: ll [filename]");
		return 0;
	}	
	
	if(argc == 2)
		dir_info = opendir(argv[1]);
		
	if(argc == 1)
		dir_info = opendir(".");
		
	if (NULL != dir_info)
	{
		while ((dir_entry = readdir(dir_info)))  
		{
			
			if(argc == 2)
				strcpy(pathName, argv[1]);
			if(argc == 1)
				strcpy(pathName, ".");
			strcat(pathName, "/");
			strcat(pathName, dir_entry->d_name);
			stat(pathName, &file_info);
			
			//check directory
			if(S_ISDIR(file_info.st_mode))
				printf("d");
			else
				printf("-");
				
			//check permission to user
			if(file_info.st_mode & S_IRUSR)
				printf("r");
			else
				printf("-");
			if(file_info.st_mode & S_IWUSR)
				printf("w");
			else
				printf("-");
			if(file_info.st_mode & S_IXUSR)
				printf("x");
			else
				printf("-");
			
			//check permission to group
			if(file_info.st_mode & S_IRGRP)
				printf("r");
			else
				printf("-");
			if(file_info.st_mode & S_IWGRP)
				printf("w");
			else
				printf("-");
			if(file_info.st_mode & S_IXGRP)
				printf("x");
			else
				printf("-");
				
			//check permission to other
			if(file_info.st_mode & S_IROTH)
				printf("r");
			else
				printf("-");
			if(file_info.st_mode & S_IWOTH)
				printf("w");
			else
				printf("-");
			if(file_info.st_mode & S_IXOTH)
				printf("x");
			else
				printf("-");
				
			printf(" %d", file_info.st_nlink);
			user = getpwuid(file_info.st_uid);
			printf(" %s", user->pw_name);
			printf(" %s", getgrgid(getgid())->gr_name);
			printf(" %5d", (int)file_info.st_size);
			printf(" %d월", localtime(&(file_info.st_mtime))->tm_mon);
			printf(" %2d", localtime(&(file_info.st_mtime))->tm_mday);
			printf(" %d:", localtime(&(file_info.st_mtime))->tm_hour);
			printf("%02d", localtime(&(file_info.st_mtime))->tm_min);
			printf(" %s", dir_entry->d_name);
			printf("\n");
			
		}
		closedir(dir_info);
	}
	return 0;
}

int copy_file(int argc, char **argv)
{
	char buf[256];
	int rfd = open(argv[1], O_RDONLY);
	int wfd = open(argv[2], O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR | S_IWUSR);
	
	int readn;
	
	if(argc < 3 || argc > 4){
		printf("Usage: cp [fileTobeCopied] [clonefile]");
		return 0;
	}
		
	if(rfd == -1 || wfd == -1){
		printf("FILE OPEN ERROR");
		return 0;
	}
	
	while((readn = read(rfd, buf, sizeof(buf))) > 0){
		if(write(wfd, buf, readn) == -1)
			printf("File Write Error");
	}
	
	close(rfd);
	close(wfd);
	return 0;
}

int remove_file(int argc, char **argv)
{
	if(argc < 2 || argc > 3){
		printf("Usage: rm [fileTobeRemoved]");
		return 0;
	}
	
	if(unlink(argv[1]) == -1)
		printf("cannot find file");
	return 0;
}


int move_file(int argc, char **argv)
{
	if(argc < 3 || argc > 4){
		printf("Usage: mv [fileTobeMoved] [destination]");
		return 0;
	}

	rename(argv[1], argv[2]);
	return 0;
}


int change_directory(int argc, char **argv)
{
	if(argc < 2 || argc > 3){
		printf("Usage: cd [directory]");
		return 0;
	}
		
	if(chdir(argv[1]) == -1)
		printf("cannot find directory");
	return 0;
}

int print_working_directory(int argc)
{
	char buf[1024];
	if(argc > 2){
		printf("Usage: pwd");
		return 0;
	}
	
	getcwd(buf, sizeof(buf));
	printf("%s", buf);
	return 0;
}


int make_directory(int argc, char **argv)
{
	if(argc < 2 || argc > 3){
		printf("Usage: mkdir [directoryName]");
		return 0;
	}
		
	mkdir(argv[1], S_IRUSR | S_IWUSR | S_IXUSR);
	return 0;
}


int remove_directory(int argc, char **argv)
{
	if(argc < 2 || argc > 3){
		printf("Usage: rmdir [directoryName]");
		return 0;
	}
		
	if(rmdir(argv[1]) == -1)
		printf("cannot find directory");
	return 0;
}


int copy_directory(int argc, char **argv)
{
	int count = 0;
	DIR* dir_info;
	struct dirent  *dir_entry;
	struct stat file_info;
	char** str;
	pthread_t threads[MAXTHREAD];
	char pathName[1024];
	
	if(argc != 3){
		printf("Usage: dcp [source dir] [dest dir]");
		return 0;
	}
	
	//open source dir
	if(argc == 3){
		
		dir_info = opendir(argv[1]);
		if(dir_info == NULL)
			printf("source open error\n");
		if(opendir(argv[2]) == NULL){ 
			mkdir(argv[2], S_IRUSR | S_IWUSR | S_IXUSR);
		}
	}

	if (NULL != dir_info)
	{
		while ((dir_entry = readdir(dir_info)))
		{
			
			
			strcpy(pathName, argv[1]);
			strcat(pathName, "/");
			strcat(pathName, dir_entry->d_name);
			stat(pathName, &file_info);
			
			if(stat(pathName, &file_info) == -1){
				printf("stat error\n");
			}
			if(count >= MAXTHREAD){
				while(count > 0){
					pthread_join(threads[--count], 0);
				}
			}
			
			//copy if not directory
			if(!S_ISDIR(file_info.st_mode)){
				str = (char**)malloc(sizeof(char*) * 2);
				str[0] = (char*)malloc(sizeof(char)*1024);
				str[1] = (char*)malloc(sizeof(char)*1024);
				
				
				//복사 파일 경로, 복사할 위치를 넘겨준다.
				strcpy(str[0], argv[1]);
				strcat(str[0], "/");
				strcat(str[0], dir_entry->d_name);
				
				strcpy(str[1], argv[2]);
				strcat(str[1], "/");
				strcat(str[1], dir_entry->d_name);
				
				pthread_create(&threads[++count], NULL, copy, str);
			}
		}
		closedir(dir_info);
	
		while(count > 0){
			pthread_join(threads[--count], 0);
			
		}
	}
	
	return 0;
}

void* copy(void *arg){
	
	static int count = 0;
	int readn;
	char buf[1024];
	int rfd;
	int wfd;
	char** myarg = (char**)arg;
	printf("Thread[%d]: copy \"%s\" into \"%s\"\n", count++, myarg[0], myarg[1]);
	rfd = open(myarg[0], O_RDONLY);
	wfd = open(myarg[1], O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR | S_IWUSR);
	
	if(rfd == -1 || wfd == -1){
		printf("fileopen error\n");
		pthread_exit((void*)NULL);
	}
	
	while((readn = read(rfd, buf, sizeof(buf))) > 0){
		if(write(wfd, buf, readn) == -1)
			printf("File Write Error");
	}
	free(myarg[0]);
	free(myarg[1]);
	free(myarg);
	
	pthread_exit((void*)NULL);
}
