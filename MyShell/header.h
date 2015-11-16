#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <grp.h>
#include <pthread.h>

#define MAXLINE		1024
#define MAXARGS		128
#define MAXPATH		1024
#define MAXTHREAD		10

#define DEFAULT_FILE_MODE	0664
#define DEFAULT_DIR_MODE		0775

pthread_mutex_t thread_count_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_arg_lock = PTHREAD_MUTEX_INITIALIZER;

char prompt[] = "myshell> ";
const char delim[] = " \t\n";
int red_check = 0;
int bg_check = 0;
int pipe_check = 0;
char *red_arg;
char *pipe_arg[MAXARGS];
int pipe_arg_count = 0;


//functions
void myshell_error(char *err_msg);
int process_cmd(char *cmdline);
int parse_line(char *cmdline, char **argv);
int builtin_cmd(int argc, char **argv);

//inner functions
int list_files(int argc, char **argv);
int list_files_long(int argc, char **argv);
int copy_file(int argc, char **argv);
int remove_file(int argc, char **argv);
int move_file(int argc, char **argv);
int change_directory(int argc, char **argv);
int print_working_directory(int argc);
int make_directory(int argc, char **argv);
int remove_directory(int argc, char **argv);
int copy_directory(int argc, char **argv);
int redirect(char* red_file);
void* copy(void *argv);
