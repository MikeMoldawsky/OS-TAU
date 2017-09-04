/*
 * message_reader.c
 *
 *  Created on: Jun 24, 2017
 *      Author: Mike Moldawsky
 */

#include "message_slot.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#define ARG_NUM									2

#define INVALID_GENERAL_ARG_STR 				"Invalid number (%d) of arguments - should be %d arguments.\n"
#define INVALID_SIZE_INPUT_STR 					"Input: <%s> - is invalid\nstrtol() function failed to convert from string to integer.\n"
#define OPEN_ERROR_STR							"Can't open file descriptor  <%s>\nError reading: %s\n"
#define CLOSE_ERROR_STR							"Can't close file descriptor <%s>\nError closing: %s\n"
#define READ_ERROR_STR							"Can't read file descriptor  <%s>\nError reading: %s\n"
#define IOCTL_ERROR_STR							"Failure in ioctl() function.\nError reading: %s\n"
#define MSG_SUCCESS_STR							"Message received From channel number <%d> is <%d> bytes long\nMessage: %s\n"


#define INVALID_GENERAL_ARGC_MSG(num) 			printf(INVALID_GENERAL_ARG_STR, num-1, ARG_NUM-1);
#define INVALID_SIZE_MSG(inputSize)				printf(INVALID_SIZE_INPUT_STR, inputSize);
#define INVALID_GENERAL_ARGC_MSG(num) 			printf(INVALID_GENERAL_ARG_STR, num-1, ARG_NUM-1);
#define OPEN_ERROR_MSG(path)					printf(OPEN_ERROR_STR, path, strerror( errno ));
#define CLOSE_ERROR_MSG(path)					printf(CLOSE_ERROR_STR, path, strerror( errno ));
#define READ_ERROR_MSG(path)					printf(READ_ERROR_STR, path, strerror( errno ));
#define IOCTL_ERROR_MSG()						printf(IOCTL_ERROR_STR, strerror( errno ));
#define MSG_SUCCESS_MSG(chnl, msg)			printf(MSG_SUCCESS_STR, chnl, strlen(msg), msg);

#define CLOSE_FILE(fd, path)\
		if (close(fd) == ERROR){\
			CLOSE_ERROR_MSG(path);\
			return ERROR;\
		}

typedef struct args{
	int channel_index;
}Args;

int parse_cli(int argc, char **argv, Args* args);

int main(int argc, char **argv) {
	int fd;
	char msg[BUF_SIZE+1] = {0};
	Args args;
	if (parse_cli(argc, argv, &args) != SUCCESS) /*invalid arguments*/
		return ERROR;
	if((fd = open(DEVICE_PATH, O_RDONLY)) < 0){
		OPEN_ERROR_MSG(DEVICE_PATH);
		return ERROR;
	}

	/*sets device message slot to the right channel */
	if(ioctl(fd, IOCTL_SET_MSG_CHNL, args.channel_index) < 0){
		IOCTL_ERROR_MSG();
		CLOSE_FILE(fd, DEVICE_PATH);
		return ERROR;
	}

	/*reading message from device message slot channel */
	/*No loop in read - reading entire buffer or failure*/
	if (read(fd, msg, BUF_SIZE) < 0){
		READ_ERROR_MSG(DEVICE_PATH);
		CLOSE_FILE(fd, DEVICE_PATH);
		return ERROR;
	}

	CLOSE_FILE(fd, DEVICE_PATH)

	MSG_SUCCESS_MSG(args.channel_index, msg);
	return SUCCESS;

}

int parse_cli(int argc, char **argv, Args* args){
	/*Invalid num of arguments*/
	char *err_check;
	if (argc != ARG_NUM){
		INVALID_GENERAL_ARGC_MSG(argc);
		return ERROR;
	}

	errno = SUCCESS; /*sets errno to success to find if strtol failed*/
	args->channel_index = strtol(argv[1], &err_check, 10);
	if(errno != SUCCESS || *err_check != '\0'){
		INVALID_SIZE_MSG(argv[1]);
		return ERROR;
	}
	return SUCCESS;
}




