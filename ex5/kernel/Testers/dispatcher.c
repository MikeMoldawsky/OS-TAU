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

#define EXEC_READER						    "./reader"
#define EXEC_SENDER						    "./sender"

#define FORK_ERROR_STR 						"Fork function failure: %s.\n"
#define EXECV_ERROR_STR						"Execv function failed failed: %s\n"

#define FORK_ERROR_MSG()					printf(FORK_ERROR_STR, strerror(errno));
#define EXECV_ERROR_MSG()					printf(EXECV_ERROR_STR, strerror(errno))

int main(int argc, char **argv) {
	int arg_num = 2, is_read, i = 0, err1 = 0;
	pid_t pid;
	char *read_params[] = {EXEC_READER, NULL, NULL, NULL};
	char *write_params[] = {EXEC_SENDER, NULL, NULL, NULL, NULL};

	if(argv[1][0] == 'r' )
		is_read = 1;
	else if(argv[1][0] == 'w' || argv[1][0] == 's' )
			is_read = 0;
	else{
		printf("first input is {r|w} read or write or {s} for send\n");
		return ERROR;
	}

	while(arg_num < argc){
		pid = fork();
		if (pid == ERROR){
			FORK_ERROR_MSG();
			return ERROR;
		}
		if (pid == 0){/*child process*/
			/*setting param for count execution*/
			if(is_read == 1){
				for(i = 1; i < 3; i++) read_params[i] = argv[arg_num++];
				execv(EXEC_READER, read_params);

			}else{
				for(i = 1; i < 4; i++) 	write_params[i] = argv[arg_num++];
				execv(EXEC_SENDER, write_params);
			}

			EXECV_ERROR_MSG();
			printf("Input is in the format:\nargv[1]={r | {w | s} }\n");
			printf("if read: tuples of\t<channel, filename> ......<channel, filename>\n");
			printf("if write: triples of\t<channel, msg, filename> ...... <channel, msg, filename>\n");
			return ERROR;
		}else{
			if(is_read) arg_num += 2;
			else arg_num += 3;
		}
	}

	/*waiting for all child processes*/
	while(wait(&err1) != -1){
		if(WEXITSTATUS(err1) != SUCCESS || WIFEXITED(err1) == 0){
			printf("At least 1 process failed.\n");
			return ERROR;
		}
	}
	return SUCCESS;
}



