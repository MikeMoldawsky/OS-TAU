#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#define SUCCESS 								0
#define ERROR									-1
#define ERROR_FILE								-1	/*read system call convention*/
#define ERROR_INVALID_INPUT 					-2

#define ARG_NUM									5
#define MAX_FILE_NAME 							1024

#define PIPE_PATH_STR							"/tmp/counter_%d"

#define OPEN_ERROR_STR										"Can't open %s\nError opening file: %s\n"
#define CLOSE_ERROR_STR										"Can't close %s\nError closing file: %s\n"
#define WRITE_ERROR_STR										"Can't write %s\nError writing file: %s\n"
#define SIZE_INPUT_STR										"Input: %s - is invalid\natoi function failed to convert from string to integer"
#define INVALID_GENERAL_ARG_STR 							"Invalid number (%d) of arguments - should be %d arguments.\n"
#define INVALID_CHAR_ARG_STR								"Invalid character count argument <%s> - argument should be of length 1.\n"
#define MMAP_ERROR_STR										"Error mmapping the file: %s.\n"
#define MUNMAP_ERROR_STR 									"Error un-mmapping the file: %s.\n"
#define SPRINTF_ERROR_STR									"Error in sprintf() function.\n"
#define MKFIFO_ERROR_STR									"Error while creating named pipe file: %s\nError: %s.\n"
#define UNLINK_ERROR_STR									"Error in unlink function for deleting named pipe file: %s\nError: %s.\n"
#define KILL_ERROR_STR										"Error in kill function: %s\n"


#define OPEN_ERROR_MSG(path)								printf(OPEN_ERROR_STR, path, strerror( errno ));
#define CLOSE_ERROR_MSG(path)								printf(CLOSE_ERROR_STR, path, strerror( errno ));
#define INVALID_GENERAL_ARGC_MSG(num) 			        	printf(INVALID_GENERAL_ARG_STR, num, ARG_NUM);
#define INVALID_CHAR_ARG_MSG(str)	 			        	printf(INVALID_CHAR_ARG_STR, str);
#define INVALID_SIZE_MSG(inputSize)							printf(SIZE_INPUT_STR, inputSize);
#define WRITE_ERROR_MSG(path)								printf(WRITE_ERROR_STR, path, strerror( errno ));
#define MMAP_ERROR_MSG()									printf(MMAP_ERROR_STR, strerror(errno));
#define MUNMAP_ERROR_MSG()									printf(MUNMAP_ERROR_STR, strerror(errno));
#define SPRINTF_ERROR_MSG()									printf(SPRINTF_ERROR_STR);
#define MKFIFO_ERROR_MSG(path)								printf(MKFIFO_ERROR_STR, path, strerror(errno))
#define UNLINK_ERROR_MSG(path)		 						printf(UNLINK_ERROR_STR, path, strerror(errno));
#define  KILL_ERROR_MSG()									printf(KILL_ERROR_STR, strerror(errno));

#define POS_MIN(e1, e2)										((e1) < (e2)) && ((e1) >= 0) ? (e1) : (e2);

#define UNLINK_FILE(path)\
		if(unlink(path) < 0){\
			UNLINK_ERROR_MSG(path);\
			return errno;\
		}

#define CLOSE_FILE(fd, path)\
		if (close(fd) == ERROR_FILE){\
			CLOSE_ERROR_MSG(path);\
			return errno;\
		}

#define ASSERT_OPEN_FILE(fd, path)\
		if (fd == ERROR_FILE){\
			OPEN_ERROR_MSG(path);\
			return errno;\
		}

#define ASSERT_OPEN_PIPE(fd, path)\
		if (fd == ERROR_FILE){\
			OPEN_ERROR_MSG(path);\
			UNLINK_FILE(path)\
			return errno;\
		}

#define WRITE_ASSERT_SUCCESS(fdis, buff, write_size, path)\
		if ((write(fdis, buff, write_size)) != write_size){\
			WRITE_ERROR_MSG(path);\
			CLOSE_FILE(fdis,path) /*try to close*/\
			UNLINK_FILE(path);\
			return errno;\
		}

typedef struct args{
	char character;
	char* path;
	ssize_t off_set;
	ssize_t chunk_size;
}Args;

int parse_cli(int argc, char **argv, Args* args);

int main(int argc, char **argv) {
	int status, fd, cnt = 0, page_size = getpagesize(), size_to_map;
	char *file_map, pipe_path[MAX_FILE_NAME];
	Args args;
	off_t cur_off_set, chars_to_read;
	pid_t pid = getpid(), parent_pid = getppid();

	if ((status = parse_cli(argc,argv, &args)) != SUCCESS) /*invalid arguments*/
			return status;

	/*open file*/
	fd = open(args.path, O_RDONLY, 0777);
	ASSERT_OPEN_FILE(fd, args.path);

	/*Start reading file*/
	cur_off_set = args.off_set;
	chars_to_read = args.chunk_size;

	while( 0 < chars_to_read ){

			size_to_map = POS_MIN(chars_to_read, page_size); /*shouldn't read more than necessary*/
			if(size_to_map == 0)
				break;
			/*map page size from file into RAM*/
			if ((file_map = (char*)mmap(NULL, size_to_map, PROT_READ, MAP_SHARED, fd, cur_off_set)) == MAP_FAILED){
				MMAP_ERROR_MSG();
				CLOSE_FILE(fd, args.path);
				return errno;
			}

			/*counting the number of target char in specific part*/
			for (int i = 0; i < size_to_map; ++i) {
				if(file_map[i] == args.character)
					cnt++;
			}

			/*updating chars left to read*/
			chars_to_read -= size_to_map;
			cur_off_set += size_to_map;

			/*un map page size from file into RAM*/
			if(munmap(file_map, size_to_map) == ERROR) {
				MUNMAP_ERROR_MSG();
				CLOSE_FILE(fd, args.path);
				return errno;
			}

	}

	CLOSE_FILE(fd, args.path); /*done with input file*/

	/*-----Start dealing with sending result to parent process------*/

	if(sprintf(pipe_path, PIPE_PATH_STR, pid) < 0){
		SPRINTF_ERROR_MSG();
		return ERROR;
	}
	/*creating name pipe file to send result to parent*/
	if(mkfifo(pipe_path, 0777) < 0){
		MKFIFO_ERROR_MSG(pipe_path);
		return errno;
	}
	/*signaling parent that this process is ready for sending result*/
	if(kill(parent_pid, SIGUSR1) != SUCCESS){
		KILL_ERROR_MSG();
		UNLINK_FILE(pipe_path);
		return errno;
	}

	/*writing to pipe*/
	fd = open(pipe_path, O_WRONLY);
	ASSERT_OPEN_PIPE(fd, pipe_path);
	WRITE_ASSERT_SUCCESS(fd, &cnt, sizeof(cnt), pipe_path); /*writing to pipe the result*/

	/*Waiting for parent process to finish as written in "The flow"*/
	sleep(1);

	/*Done with named pipe*/
	CLOSE_FILE(fd, pipe_path);
	UNLINK_FILE(pipe_path);
	return SUCCESS;
}


int parse_cli(int argc, char **argv, Args* args){
	char* arg;

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
	args->off_set = atoi(argv[3]);
	args->chunk_size = atoi(argv[4]);

	if((args->off_set == 0 && strcmp(argv[3],"0") != 0)  || args->chunk_size == 0 )
	{
		arg = (args->off_set == 0) ? argv[3] : argv[4]; /*setting right error to print*/
		INVALID_SIZE_MSG(arg);
		return ERROR_INVALID_INPUT;
	}

	return SUCCESS;
}



