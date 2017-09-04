#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SUCCESS 								0
#define ERROR									-1
#define ERROR_FILE								-1	/*read system call convention*/
#define ERROR_INVALID_INPUT 					-2

#define ARG_NUM									3
#define MAX_PROCESS_NUM							16
#define MAX_FILE_NAME 							1024

#define EXEC_COUNTER_PATH					    "./counter"
#define PIPE_PATH_STR							"/tmp/counter_%d"

#define OPEN_ERROR_STR										"Can't open %s\nError opening file: %s\n"
#define CLOSE_ERROR_STR										"Can't close %s\nError closing file: %s\n"
#define READ_ERROR_STR										"Can't read %s\nError reading file: %s\n"
#define INVALID_GENERAL_ARG_STR 							"Invalid number (%d) of arguments - should be %d arguments.\n"
#define INVALID_CHAR_ARG_STR								"Invalid character count argument <%s> - argument should be of length 1.\n"
#define SPRINTF_ERROR_STR									"Error in sprintf() function.\n"
#define STAT_ERROR_STR										"Can't get stat in %s\nError with stat file: %s\n"
#define SIGACTION_ERROR_STR 								"Signal handle registration failed. %s\n"
#define FORK_ERROR_STR										"Fork function failure: %s.\n"
#define EXECV_ERROR_STR										"Execv function failed failed: %s\n"
#define RESULT_STR											"Result:\nThe letter <%c> count is: %d\n"
#define PROCESS_FAILURE_STR									"Error in one of child process %s.\n"

#define OPEN_ERROR_MSG(path)								printf(OPEN_ERROR_STR, path, strerror( errno ));
#define CLOSE_ERROR_MSG(path)								printf(CLOSE_ERROR_STR, path, strerror( errno ));
#define INVALID_GENERAL_ARGC_MSG(num) 			        	printf(INVALID_GENERAL_ARG_STR, num, ARG_NUM);
#define INVALID_CHAR_ARG_MSG(str)	 			        	printf(INVALID_CHAR_ARG_STR, str);
#define READ_ERROR_MSG(path)								printf(READ_ERROR_STR, path, strerror( errno ));
#define SPRINTF_ERROR_MSG()									printf(SPRINTF_ERROR_STR);
#define STAT_ERROR_MSG(path)								printf(STAT_ERROR_STR, path, strerror( errno ));
#define SIGACTION_ERROR_MSG() 								printf(SIGACTION_ERROR_STR, strerror(errno))
#define FORK_ERROR_MSG()									printf(FORK_ERROR_STR, strerror(errno));
#define EXECV_ERROR_MSG()									printf(EXECV_ERROR_STR, strerror(errno))
#define RESULT_MSG(c, count)								printf(RESULT_STR, c, count);
#define PROCESS_FAILURE_MSG() 								printf(PROCESS_FAILURE_STR, strerror(errno));

#define MIN(e1, e2)											((e1) < (e2)) ? (e1) : (e2);

typedef struct args{
	char character;
	char* path;
}Args;


int setChunkAndProcessNum(off_t size, int* processes_num, int* chunk);
int parse_cli(int argc, char **argv, Args* args);
void my_signal_handler(int signum, siginfo_t* info, void* ptr);

/*global variable for result*/
int result = 0;

int main(int argc, char **argv) {
	Args args;
	int processes_num, chunk, cur_chunk, err1, err2;
	pid_t pid;
	char offset_str[MAX_FILE_NAME],chunk_str[MAX_FILE_NAME];
	char *params[] = {EXEC_COUNTER_PATH, NULL, NULL, NULL, NULL, NULL};
	ssize_t cur_offset = 0;
	struct stat status;
	struct sigaction new_action;

	if (parse_cli(argc,argv, &args) != SUCCESS) /*invalid arguments*/
		return ERROR;

	/*Connecting signal handler*/
	memset(&new_action, 0, sizeof(new_action));
	new_action.sa_sigaction = my_signal_handler;
	new_action.sa_flags = SA_SIGINFO;
	if( sigaction(SIGUSR1, &new_action, NULL) != SUCCESS){
		SIGACTION_ERROR_MSG();
		return ERROR;
	}

	/*get file stats*/
	if (stat(args.path, &status) < 0){
		STAT_ERROR_MSG(args.path);
		return errno;
	}

	setChunkAndProcessNum(status.st_size, &processes_num, &chunk);
	params[2]= args.path; /*setting param for count execution*/

	for (int i = 0; i < processes_num; ++i) {
		cur_chunk = MIN(status.st_size-cur_offset, chunk);
		pid = fork();
		if (pid == ERROR){
			FORK_ERROR_MSG();
			return ERROR;
		}
		if (pid == 0){/*child process*/
			err1 = sprintf(offset_str, "%d", cur_offset);
			err2 = sprintf(chunk_str, "%d", cur_chunk);
			if(err1 < 0 || err2 < 0){
				SPRINTF_ERROR_MSG();
				return ERROR;
			}
			/*setting param for count execution*/
			params[1] = &args.character;
			params[3] = offset_str;
			params[4] = chunk_str;
			execv(EXEC_COUNTER_PATH, params);
			EXECV_ERROR_MSG();
			return ERROR;
		}
		else{ /*parent process*/
			cur_offset += cur_chunk;
			sleep(2);
		}
	}

	/*waiting for all child processes*/
	while(wait(&err1) != -1){
		if(WEXITSTATUS(err1) != SUCCESS || WIFEXITED(err1) == 0){
			PROCESS_FAILURE_MSG();
			return ERROR;
		}
	}

	RESULT_MSG(args.character, result);
	return SUCCESS;
}

int setChunkAndProcessNum(off_t size, int* processes_num, int* chunk){
	int page_size = getpagesize(), cur_processes_num = MAX_PROCESS_NUM+1, cur_chunk = page_size;

	if(size < 2*page_size){
		*processes_num = 1;
		*chunk = size;
		return SUCCESS;
	}
	while (cur_processes_num > MAX_PROCESS_NUM){
		cur_chunk *= 2;
		cur_processes_num = size/cur_chunk;
	}

	/*last process will have a chunk that is less than cur_chunk*/
	if (size%cur_chunk !=0 && cur_processes_num < MAX_PROCESS_NUM)
		cur_processes_num++;

	*processes_num = cur_processes_num;
	*chunk = cur_chunk;
	return SUCCESS;
}

int parse_cli(int argc, char **argv, Args* args){
	/*Invalid num of arguments*/
	if (argc != ARG_NUM)
	{
		INVALID_GENERAL_ARGC_MSG(argc);
		return ERROR_INVALID_INPUT;
	}

	/*character count should be of length 1*/
	if(strlen(argv[1]) != 1)
	{
		INVALID_CHAR_ARG_MSG(argv[1]);
		return ERROR_INVALID_INPUT;
	}
	args->character= argv[1][0];
	args->path = argv[2];

	return SUCCESS;
}

void my_signal_handler(int signum, siginfo_t* info, void* ptr){
	int fd, cnt;
	char pipe_path[MAX_FILE_NAME];

	if(sprintf(pipe_path, PIPE_PATH_STR, info->si_pid) < 0){
		SPRINTF_ERROR_MSG();
		return;
	}

	/*opening pipe*/
	if ((fd = open(pipe_path, O_RDONLY)) == ERROR_FILE){
				OPEN_ERROR_MSG(pipe_path);
	}
	/*reading from pipe the result*/
	if (read(fd, &cnt, sizeof(cnt)) != sizeof(cnt)){
		READ_ERROR_MSG(pipe_path);\
		if (close(fd) == ERROR_FILE){
			CLOSE_ERROR_MSG(pipe_path)
			return;
		}
		return;
	}
	/*closing pipe*/
	if (close(fd) == ERROR_FILE){
		CLOSE_ERROR_MSG(pipe_path);
		return;
	}
	/*updating global variable*/
	result += cnt;
	return;
}

