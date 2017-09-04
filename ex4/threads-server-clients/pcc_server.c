#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define SUCCESS 								0
#define ERROR									-1
#define ERROR_FILE								-1	/*read system call convention*/

#define NO_FILE									-8
#define NO_PATH									""

#define SIZE_INCREASE_UNIT 						1024
#define SIZE_BYTE 								1
#define SIZE_KB 								((SIZE_INCREASE_UNIT) * (SIZE_BYTE))
#define SIZE_MB 								((SIZE_INCREASE_UNIT) * (SIZE_KB))

#define SIZE_BUFF_BIG							((SIZE_KB) * 4)
#define SIZE_BUFF_SMALL							(SIZE_KB)

#define TRUE									1
#define FALSE									0

#define SERVER_PORT								2233
#define MAX_IN_QUEUE							10
#define PRINT_LOWER_BOUND						32
#define PRINT_UPPER_BOUND						126
#define SIZE_ASCII								(PRINT_UPPER_BOUND-PRINT_LOWER_BOUND+1)

#define MY_IP									"127.0.0.1"
#define SOCKET_STR								"SOCKET"
#define CLIENT_CON_STR							"CLIENT CONNECTION"

#define MUTEX_DESTROY_ERROR_STR					"ERROR in pthread_mutex_destroy(): %s.\n"
#define MUTEX_INIT_ERROR_STR					"ERROR in pthread_mutex_init(): %s.\n"
#define MUTEX_UNLOCK_ERROR_STR					"ERROR in pthread_mutex_unlock(): %s.\n"
#define MUTEX_LOCK_ERROR_STR					"ERROR in pthread_mutex_lock(): %s.\n"
#define THREAD_CREATE_ERROR_STR					"ERROR in pthread_create(): %s.\n"
#define SIGACTION_ERROR_STR 					"Signal handle registration failed. %s\n"
#define	ACCEPT_ERROR_STR						"ERROR in accept(): %s.\n"
#define SOCKET_ERROR_STR						"ERROR in socket(): %s.\n"
#define BIND_ERROR_STR 							"ERROR bind(): %s.\n"
#define LISTEN_ERROR_STR						"ERORR in listen(): %s.\n"
#define CLOSE_ERROR_STR							"Can't close file descriptor <%s>\nError closing: %s\n"
#define READ_ERROR_STR							"Can't read file descriptor  <%s>\nError reading: %s\n"
#define WRITE_ERROR_STR							"Can't write %s\nError writing file: %s\n"
#define CONNECTION_ERROR_STR					"IO 0 bytes - Client connection disconnected or bad network\nclosing connection...\n."
#define SERVER_RUN_STR							"Server is running...\n"
#define MEM_ERROR_STR							"Memory allocation failure.\n"

#define GLOBAL_BYTES_READ_STR					"\nServer read total of <%d> Bytes.\n"
#define GLOBAL_PRINTABLE_FORMAT_STR				"Result format is:\n(ASCII_NUMBER, Char: CHARACTER): <#TIMES_APPEARED>.\nResults:\n"
#define GLOBAL_PRINTABLE_READ_STR				"(%d,Char: '%c'): \t%d.\n"

#define MEM_ERROR_MSG()							printf(MEM_ERROR_STR);
#define MUTEX_DESTROY_ERROR_MSG(num)			printf(MUTEX_DESTROY_ERROR_STR, strerror(num));
#define MUTEX_INIT_ERROR_MSG(num)				printf(MUTEX_INIT_ERROR_STR, strerror(num));
#define MUTEX_UNLOCK_ERROR_MSG(num)				printf(MUTEX_UNLOCK_ERROR_STR, strerror(num));
#define MUTEX_LOCK_ERROR_MSG(num) 				printf(MUTEX_LOCK_ERROR_STR, strerror(num));
#define THREAD_CREATE_ERROR_MSG(num) 			printf(THREAD_CREATE_ERROR_STR, strerror(num));
#define SIGACTION_ERROR_MSG() 					printf(SIGACTION_ERROR_STR, strerror(errno));
#define ACCEPT_ERROR_MSG()						printf(ACCEPT_ERROR_STR, strerror(errno));
#define SOCKET_ERROR_MSG()						printf(SOCKET_ERROR_STR, strerror( errno ));
#define BIND_ERROR_MSG()						printf(BIND_ERROR_STR, strerror(errno));
#define LISTEN_ERROR_MSG()						printf(LISTEN_ERROR_STR, strerror(errno));
#define CLOSE_ERROR_MSG(path)					printf(CLOSE_ERROR_STR, path, strerror( errno ));
#define READ_ERROR_MSG(path)					printf(READ_ERROR_STR, path, strerror( errno ));
#define WRITE_ERROR_MSG(path)					printf(WRITE_ERROR_STR, path, strerror( errno ));
#define CONNECTION_ERROR_MSG()					printf(CONNECTION_ERROR_STR);
#define SERVER_RUN_MSG()						printf(SERVER_RUN_STR);

#define GLOBAL_BYTES_READ_MSG(bytes)			printf(GLOBAL_BYTES_READ_STR, bytes);
#define GLOBAL_PRINTABLE_READ_MSG(chr, cnt)		printf(GLOBAL_PRINTABLE_READ_STR, chr, chr, cnt);
#define GLOBAL_PRINTABLE_FORMAT_MSG()			printf(GLOBAL_PRINTABLE_FORMAT_STR);

#define CLOSE_FILES(fd1,p1, fd2, p2)\
	if (close(fd1) == ERROR_FILE){\
		CLOSE_ERROR_MSG(p1);\
		CLOSE_FILE(fd2, p2);\
		return errno;\
	}\
	CLOSE_FILE(fd2, p2); /*trying to close second file*/\

#define CLOSE_FILE(fd, path)\
	if(fd != NO_FILE){\
		if (close(fd) == ERROR_FILE){\
			CLOSE_ERROR_MSG(path);\
			exit(ERROR);\
		}\
	}

#define CLOSE_SOCKET(fd)\
	if (g_socket_closed == FALSE){\
		CLOSE_FILE(fd, SOCKET_STR);\
		g_socket_closed = TRUE;\
	}


#define EXIT_AND_RELEASE_RESOURCES(fd, str1, fd2, str2)\
		CLOSE_FILES(fd, str1, fd2, str2);\
		return ERROR;

#define EXIT_RELEASE_CLIENT(fd1, ptr)\
		free(ptr);\
		CLOSE_FILE(fd1, CLIENT_CON_STR);\
		exit(ERROR);

#define MUTEX_DESTROY(arr, j, rc1)\
		for (j = 0; j < SIZE_ASCII ; ++j) {\
			if((rc1 = pthread_mutex_destroy(&arr[j])) != SUCCESS)\
				MUTEX_DESTROY_ERROR_MSG(rc1);\
		}


typedef struct statis{
	int ascii[SIZE_ASCII];
	int bytes_read;
} Stat;


void my_signal_handler(int signum);
void * client_counter(void* client_fd);
int addStats(Stat* local_stat);

/*global variables*/
int g_socket_fd = -1;
int g_socket_closed = TRUE;
int g_thread_cnt = 0;
Stat g_statistics = {{0}};
pthread_mutex_t g_ascii_locks[SIZE_ASCII];

int main(int argc, char **argv) {
	int conn_fd, rc, i;
	pthread_t thread;
	struct sockaddr_in serv_addr = {0};
	struct sigaction new_action = {0};

	/*Connecting signal handler*/
	new_action.sa_handler = my_signal_handler;
	if(sigaction(SIGINT, &new_action, NULL) != SUCCESS){
		SIGACTION_ERROR_MSG();
		return ERROR;
	}

	/* --- Initialize mutexes for all cell in array ----*/
	for (i = 0; i < SIZE_ASCII ; ++i) {
		if((rc = pthread_mutex_init( &g_ascii_locks[i], NULL )) != SUCCESS ){
			MUTEX_INIT_ERROR_MSG(rc)
			return ERROR;
		}
	}

	/* --- Creating socket and binding it to a port & start listening ----*/
	if((g_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR){
		CLOSE_SOCKET(g_socket_fd);
		MUTEX_DESTROY(g_ascii_locks, i, rc);
		return ERROR;
	}
	g_socket_closed = FALSE;
	/*initializing server configurations*/
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY = any local machine address
	serv_addr.sin_port = htons(SERVER_PORT);
	if(bind(g_socket_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) != SUCCESS){
		BIND_ERROR_MSG();
		CLOSE_SOCKET(g_socket_fd);
		MUTEX_DESTROY(g_ascii_locks, i, rc);
		return ERROR;
	}

	if(listen(g_socket_fd, MAX_IN_QUEUE) != SUCCESS ){
	    LISTEN_ERROR_MSG();
	    CLOSE_SOCKET(g_socket_fd);
		MUTEX_DESTROY(g_ascii_locks, i, rc);
	    return ERROR;
	}

	/* Waiting for incoming connections*/
	SERVER_RUN_MSG();
	while(TRUE){
        if((conn_fd = accept(g_socket_fd, NULL, NULL)) == ERROR ){
        	ACCEPT_ERROR_MSG();
        	CLOSE_SOCKET(g_socket_fd);
        	SERVER_RUN_MSG();
    		MUTEX_DESTROY(g_ascii_locks, i, rc);
        	return ERROR;
        }

        if((rc = pthread_create(&thread, NULL, client_counter, (void *)conn_fd)) != SUCCESS){
            THREAD_CREATE_ERROR_MSG(rc);
            CLOSE_SOCKET(g_socket_fd);
            CLOSE_FILE(conn_fd, CLIENT_CON_STR);
    		MUTEX_DESTROY(g_ascii_locks, i, rc);
            return ERROR;
        }

    }
	/*shouldn't arrive here*/
    CLOSE_FILE(g_socket_fd, SOCKET_STR);
	MUTEX_DESTROY(g_ascii_locks, i, rc);
    pthread_exit(NULL);
}

void my_signal_handler(int signum){
	int i;
	char c = (char)PRINT_LOWER_BOUND;
	/*Closing socket*/
	if (g_socket_closed == FALSE && close(g_socket_fd) == ERROR){ /*close fd and checks if it was already closed*/
		CLOSE_ERROR_MSG(SOCKET_STR)
		exit(ERROR);
	}
	g_socket_closed = TRUE;
	/*Waiting for all threads to finish*/
	while(g_thread_cnt > 0);
	/*Printing global statistics of Server*/
	GLOBAL_BYTES_READ_MSG(g_statistics.bytes_read);
	GLOBAL_PRINTABLE_FORMAT_MSG();
	for (i = 0; i < SIZE_ASCII; ++i, c++) {
		GLOBAL_PRINTABLE_READ_MSG(c, g_statistics.ascii[i]);
	}
	/*Server is done working*/
	exit(SUCCESS);
}

void * client_counter(void* client_fd){
	int i, n_sent_bytes,sent_bytes, bytes_cur_read, bytes_cur_write, cnt = 0, n_cnt, left_to_transfer;
	int fd = (int)client_fd, buff_size;
	char *buff, *n_len_arg, *n_sum_printable;
	Stat local_stat = {{0}};

	__sync_add_and_fetch(&g_thread_cnt, 1);

	/*first message - number of bytes that client is sending*/
	n_len_arg = (char*)&n_sent_bytes;
	left_to_transfer = sizeof(n_sent_bytes);
	while(left_to_transfer > 0){
		if ((bytes_cur_read = read(fd, n_len_arg, left_to_transfer)) == ERROR){
			READ_ERROR_MSG(CLIENT_CON_STR);
		    CLOSE_FILE(fd, CLIENT_CON_STR);
			exit(ERROR);
		}
		/*check for half open connection*/
		if(bytes_cur_read == 0){
			CONNECTION_ERROR_MSG();
		    CLOSE_FILE(fd, CLIENT_CON_STR);
			exit(ERROR);
		}
		left_to_transfer -= bytes_cur_read;
	}
	/*Reading from connection all client message and processing it*/
	sent_bytes = ntohl(n_sent_bytes);
	/*buffer size initialized by the size of the request*/
	if (sent_bytes >= SIZE_MB)
		buff_size = SIZE_BUFF_BIG;
	else
		buff_size = SIZE_BUFF_SMALL;

	if ((buff = (char*)malloc(sizeof(char)*buff_size)) == NULL){/*malloc fail check*/
		MEM_ERROR_MSG();
	    CLOSE_FILE(fd, CLIENT_CON_STR);
		exit(ERROR);
	}

	while(local_stat.bytes_read < sent_bytes){
		if ((bytes_cur_read = read(fd, buff, sizeof(buff))) == ERROR ){
			READ_ERROR_MSG(CLIENT_CON_STR);
			EXIT_RELEASE_CLIENT(fd, buff);
		}
		/*check for half open connection*/
		if(bytes_cur_read == 0){
			CONNECTION_ERROR_MSG();
			EXIT_RELEASE_CLIENT(fd, buff);
		}

		/*updating statistics*/
		for (i = 0; i < bytes_cur_read; ++i) {
			local_stat.bytes_read++;
			if ((PRINT_LOWER_BOUND <= buff[i]) && (buff[i] <= PRINT_UPPER_BOUND)){
				local_stat.ascii[buff[i]-PRINT_LOWER_BOUND]++;
				cnt++;
			}
		}
	}
	/*Writing result back to client*/
	n_cnt = htonl(cnt);
	n_sum_printable = (char*)&n_cnt;
	left_to_transfer = sizeof(n_cnt);
	while(left_to_transfer > 0){
		if ((bytes_cur_write = write(fd, n_sum_printable, left_to_transfer)) == ERROR){
			WRITE_ERROR_MSG(CLIENT_CON_STR);
			EXIT_RELEASE_CLIENT(fd, buff);
		}
		/*check for half open connection*/
		if(bytes_cur_write == 0){
			CONNECTION_ERROR_MSG();
			EXIT_RELEASE_CLIENT(fd, buff);
		}
		left_to_transfer -= bytes_cur_write;
	}

	/*Closing connection with client*/
	if (close(fd) == ERROR){
		CLOSE_ERROR_MSG(CLIENT_CON_STR);
		free(buff);
		exit(ERROR);
	}
	/*Updating global stats*/
	if(addStats(&local_stat) != SUCCESS){
		free(buff);
		exit(ERROR);
	}

	/*decrement thread counter*/
	__sync_add_and_fetch(&g_thread_cnt, -1);
	free(buff);
	pthread_exit(NULL); // same as: return 0;
}

int addStats(Stat* local_stat){
	int i, rc;
	/*Updating*/
	__sync_add_and_fetch(&g_statistics.bytes_read, local_stat->bytes_read);
	for (i = 0; i < SIZE_ASCII ; ++i) {
		/*Lock when updating global statistics*/
		if((rc = pthread_mutex_lock(&g_ascii_locks[i])) != SUCCESS){
			MUTEX_LOCK_ERROR_MSG(rc);
			return ERROR;
		}
		g_statistics.ascii[i] += local_stat->ascii[i];
		/*Done - Unlock statistics*/
		if((rc = pthread_mutex_unlock(&g_ascii_locks[i])) != SUCCESS){
			MUTEX_UNLOCK_ERROR_MSG(rc);
			return ERROR;
		}
	}
	return SUCCESS;
}

