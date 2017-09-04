#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define SUCCESS 								0
#define ERROR									-1
#define ERROR_FILE								-1	/*read system call convention*/
#define ERROR_INVALID_INPUT 					-2

#define SIZE_INCREASE_UNIT 						1024
#define SIZE_BYTE 								1
#define SIZE_KB 								((SIZE_INCREASE_UNIT) * (SIZE_BYTE))
#define SIZE_MB 								((SIZE_INCREASE_UNIT) * (SIZE_KB))

#define SIZE_BUFF_SMALL 						(SIZE_KB)
#define SIZE_BUFF_BIG	 						(3 * (SIZE_KB))

#define NO_FILE									-8
#define ARG_NUM									2
#define SERVER_PORT								2233

#define MAX_INT_LEN_BOUND						15

#define NO_PATH									""
#define MY_IP									"127.0.0.1"
#define FILE_PATH								"/dev/urandom"
#define SOCKET_STR								"SOCKET"

#define SIZE_INPUT_STR							"Input: <%s> - is invalid\natoi() function failed to convert from string to integer\n\tOR\nInput is smaller or equal to <0>.\n"
#define INVALID_GENERAL_ARG_STR 				"Invalid number (%d) of arguments - should be %d arguments.\n"
#define MEM_ERROR_STR							"Memory allocation failure.\n"
#define WRITE_ERROR_STR							"Can't write %s\nError writing file: %s\n"
#define CLOSE_ERROR_STR							"Can't close file descriptor <%s>\nError closing: %s\n"
#define OPEN_ERROR_STR							"Can't open file descriptor  <%s>\nError reading: %s\n"
#define READ_ERROR_STR							"Can't read file descriptor  <%s>\nError reading: %s\n"
#define SOCKET_ERROR_STR						"ERROR in socket(): %s.\n"
#define CONNECT_ERROR_STR						"ERROR in connect(): %s.\n"
#define CONNECTION_ERROR_STR					"IO 0 bytes - Server connection disconnected or bad network\nclosing connection...\n."
#define RESULT_STR								"\nTOTAL PRINTABLE = %d\n"

#define INVALID_SIZE_MSG(inputSize)				printf(SIZE_INPUT_STR, inputSize);
#define INVALID_GENERAL_ARGC_MSG(num) 			printf(INVALID_GENERAL_ARG_STR, num-1, ARG_NUM-1);
#define MEM_ERROR_MSG()							printf(MEM_ERROR_STR);
#define OPEN_ERROR_MSG(path)					printf(OPEN_ERROR_STR, path, strerror( errno ));
#define CLOSE_ERROR_MSG(path)					printf(CLOSE_ERROR_STR, path, strerror( errno ));
#define READ_ERROR_MSG(path)					printf(READ_ERROR_STR, path, strerror( errno ));
#define WRITE_ERROR_MSG(path)					printf(WRITE_ERROR_STR, path, strerror( errno ));
#define SOCKET_ERROR_MSG()						printf(SOCKET_ERROR_STR, strerror( errno ));
#define CONNECT_ERROR_MSG()						printf(CONNECT_ERROR_STR, strerror( errno ));
#define RESULT_MSG(p) 							printf(RESULT_STR, p);
#define CONNECTION_ERROR_MSG()					printf(CONNECTION_ERROR_STR);

#define MIN(e1, e2)								(e1) < (e2) ? (e1) : (e2);

#define CLOSE_FILE(fd, path)\
	if(fd != NO_FILE){\
		if (close(fd) == ERROR_FILE){\
			CLOSE_ERROR_MSG(path);\
			return errno;\
		}\
	}

#define CLOSE_FILES(fd1,p1, fd2, p2)\
	if (close(fd1) == ERROR_FILE){\
		CLOSE_ERROR_MSG(p1);\
		CLOSE_FILE(fd2, p2);\
		return errno;\
	}\
	CLOSE_FILE(fd2, p2); /*trying to close second file*/\

#define ASSERT_CREATE_FD(fd, path, ptr, fd2, str)\
		if (fd == ERROR_FILE){\
			OPEN_ERROR_MSG(path);\
			free(ptr);\
			CLOSE_FILE(fd2, str);\
			return errno;\
		}

#define EXIT_AND_RELEASE_RESOURCES(ptr, fd, str1, fd2, str2)\
		free(ptr);\
		CLOSE_FILES(fd, str1, fd2, str2);\
		return ERROR;

typedef struct args{
	int bytes;
	int n_bytes;
}Args;

int parse_cli(int argc, char **argv, Args* args);


int main(int argc, char **argv) {
	Args args;
	int fd_socket, fd, buff_size, n_printable, printable;
	int bytesRead = 0, bytesToRead, bytesCurRead, bytesCurWrite, bytesToWrite, bytesWrote, left_to_transfer;
	char *buff, *len_arg, *n_res;
	struct sockaddr_in serv_addr = {0};

	if (parse_cli(argc, argv, &args) != SUCCESS) /*invalid arguments*/
		return ERROR;

	/*buffer size initialized by the size of the request*/
	if (args.bytes >= SIZE_MB)
		buff_size = SIZE_BUFF_BIG;
	else
		buff_size = SIZE_BUFF_SMALL;

	if ((buff = (char*)malloc(sizeof(char)*buff_size)) == NULL){/*malloc fail check*/
		MEM_ERROR_MSG();
		return ERROR;
	}

	fd_socket = socket(AF_INET, SOCK_STREAM, 0);
	ASSERT_CREATE_FD(fd_socket, SOCKET_STR, buff, NO_FILE, NO_PATH);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERVER_PORT);
	serv_addr.sin_addr.s_addr = inet_addr(MY_IP);

	// connect socket to the target address
	if(connect(fd_socket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
	   CONNECT_ERROR_MSG();
	   EXIT_AND_RELEASE_RESOURCES(buff,fd_socket, SOCKET_STR, NO_FILE, NO_PATH);
	}

	fd = open(FILE_PATH, O_RDONLY, 0777);
	ASSERT_CREATE_FD(fd, FILE_PATH, buff, fd_socket, SOCKET_STR);

	/* --------READ FROM INPUT AND WRITE TO SOCKET---------*/

	/*First message is the number of bytes to send*/
	len_arg = (char*)&args.n_bytes;
	bytesToWrite = sizeof(args.n_bytes);
	while(bytesToWrite > 0){
		if ((bytesCurWrite = write(fd_socket, len_arg, bytesToWrite)) < 0 ){
			WRITE_ERROR_MSG(SOCKET_STR);
			EXIT_AND_RELEASE_RESOURCES(buff,fd, FILE_PATH, fd_socket, SOCKET_STR);
		}
		if(bytesCurWrite == 0){
			CONNECT_ERROR_MSG();
			EXIT_AND_RELEASE_RESOURCES(buff,fd, FILE_PATH, fd_socket, SOCKET_STR);
		}
		bytesToWrite -= bytesCurWrite;
	}

	/*Sending data to server*/
	bytesRead = 0;
	while( bytesRead < args.bytes ){
		/*-------Reading data from File-----*/
		/*shouldn't read more than necessary*/
		bytesToRead = MIN((args.bytes-bytesRead), buff_size);
		if ((bytesCurRead = read(fd, buff, bytesToRead)) < 0){ /*reading input file error*/
			READ_ERROR_MSG(FILE_PATH);
			EXIT_AND_RELEASE_RESOURCES(buff,fd, FILE_PATH, fd_socket, SOCKET_STR);
		}

		/*update total bytes read*/
		bytesRead += bytesCurRead;
		/*-------Passing buffer to Server------*/
		bytesToWrite = bytesCurRead;
		bytesWrote = 0;

		while(bytesToWrite > 0){
			if ((bytesCurWrite = write(fd_socket, &buff[bytesWrote], bytesToWrite)) < 0){
				WRITE_ERROR_MSG(SOCKET_STR);
				EXIT_AND_RELEASE_RESOURCES(buff,fd, FILE_PATH, fd_socket, SOCKET_STR);
			}
			if (bytesCurWrite == 0){
				CONNECTION_ERROR_MSG();
				EXIT_AND_RELEASE_RESOURCES(buff,fd, FILE_PATH, fd_socket, SOCKET_STR);
			}
			bytesToWrite -= bytesCurWrite;
			bytesWrote += bytesCurWrite;
		}
	}

	/* --------READ FROM SERVER RESULT---------*/
	n_res = (char*)&n_printable;
	left_to_transfer = sizeof(n_printable);
	while(left_to_transfer > 0){

		if ((bytesCurRead = read(fd_socket, n_res, left_to_transfer)) == ERROR){
			READ_ERROR_MSG(SOCKET_STR);
			EXIT_AND_RELEASE_RESOURCES(buff,fd, FILE_PATH, fd_socket, SOCKET_STR);
		}
		if ( bytesCurRead == 0){
			CONNECTION_ERROR_MSG();
			EXIT_AND_RELEASE_RESOURCES(buff,fd, FILE_PATH, fd_socket, SOCKET_STR);
		}
		left_to_transfer -= bytesCurRead;
	}

	printable = ntohl(n_printable);
	RESULT_MSG(printable);
	/*Success -  release resources and exit*/
	free(buff);
	CLOSE_FILES(fd, FILE_PATH, fd_socket, SOCKET_STR)
	return SUCCESS;
}

int parse_cli(int argc, char **argv, Args* args){
	/*Invalid num of arguments*/
	if (argc != ARG_NUM){
		INVALID_GENERAL_ARGC_MSG(argc);
		return ERROR_INVALID_INPUT;
	}

	args->bytes = atoi(argv[1]);
	if(args->bytes <= 0 ){
		INVALID_SIZE_MSG(argv[1]);
		return ERROR_INVALID_INPUT;
	}
	args->n_bytes = htonl(args->bytes);
	return SUCCESS;
}






