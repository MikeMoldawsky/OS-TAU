/*
 * message_sender.c
 *
 *  Created on: Jun 24, 2017
 *      Author: Mike Moldawsky
 */

#include "message_slot.h" /*Our custom definitions of IOCTL operations*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#define ARG_NUM									4

#define INVALID_GENERAL_ARG_STR 				"Invalid number (%d) of arguments - should be %d arguments.\n"
#define INVALID_SIZE_INPUT_STR 					"Input: <%s> - is invalid\nstrtol() function failed to convert from string to integer.\n"
#define WRITE_ERROR_STR							"Can't write %s\nError writing file: %s\n"
#define CLOSE_ERROR_STR							"Can't close file descriptor <%s>\nError closing: %s\n"
#define OPEN_ERROR_STR							"Can't open file descriptor  <%s>\nError reading: %s\n"
#define IOCTL_ERROR_STR							"Failure in ioctl() function.\nError reading: %s\n"
#define STATUS_STR								"Success\nSent <%d> bytes to channel <%d> in device %s .\n"

#define INVALID_GENERAL_ARGC_MSG(num) 			printf(INVALID_GENERAL_ARG_STR, num-1, ARG_NUM-1);
#define INVALID_SIZE_MSG(inputSize)				printf(INVALID_SIZE_INPUT_STR, inputSize);
#define OPEN_ERROR_MSG(path)					printf(OPEN_ERROR_STR, path, strerror( errno ));
#define CLOSE_ERROR_MSG(path)					printf(CLOSE_ERROR_STR, path, strerror( errno ));
#define WRITE_ERROR_MSG(path)					printf(WRITE_ERROR_STR, path, strerror( errno ));
#define IOCTL_ERROR_MSG()						printf(IOCTL_ERROR_STR, strerror( errno ));
#define STATUS_MSG(bytes, channel)				printf(STATUS_STR, bytes, channel, argv[3]);
#define CLOSE_FILE(fd, path)\
		if (close(fd) == ERROR){\
			CLOSE_ERROR_MSG(path);\
			return ERROR;\
		}

typedef struct args{
	int channel_index;
	char* msg;
}Args;

int parse_cli(int argc, char **argv, Args* args);

int main(int argc, char **argv) {
	int fd, wrote;
	Args args;
	if (parse_cli(argc, argv, &args) != SUCCESS) /*invalid arguments*/
		return ERROR;

	if((fd = open(argv[3], O_WRONLY)) < 0){
		OPEN_ERROR_MSG(argv[3]);
		return ERROR;
	}

	/*sets device message slot to the right channel */
	if(ioctl(fd, IOCTL_SET_MSG_CHNL, args.channel_index) < 0){
		IOCTL_ERROR_MSG();
		CLOSE_FILE(fd, argv[3])
		return ERROR;
	}

	/*writing message to device message slot channel */
	/*No loop in read - reading entire buffer or failure*/
	if ((wrote = write(fd, args.msg, strlen(args.msg))) < 0 ){
		WRITE_ERROR_MSG(argv[3]);
		CLOSE_FILE(fd, argv[3]);
		return ERROR;
	}
	CLOSE_FILE(fd, argv[3]);
	STATUS_MSG(wrote, args.channel_index);
	return SUCCESS;
}

int parse_cli(int argc, char **argv, Args* args){
	char *err_check;
	/*Invalid num of arguments*/
	if (argc != ARG_NUM){
		INVALID_GENERAL_ARGC_MSG(argc);
		return ERROR;
	}

	errno = SUCCESS;
	args->channel_index = strtol(argv[1], &err_check, 10);
	if(errno != SUCCESS || *err_check != '\0'){
		INVALID_SIZE_MSG(argv[1]);
		return ERROR;
	}
	args->msg = argv[2];
	return SUCCESS;
}


