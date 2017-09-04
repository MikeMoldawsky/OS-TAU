#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <libgen.h>
#include <time.h>

#define SUCCESS 								0
#define ERROR_FILE								-1	/*read system call convention*/
#define ERROR_SEEK								-2
#define ERROR_MEMORY							-3
#define ERROR_EMPTY_FILE						-4
#define ERROR_INVALID_INPUT 					-5
#define ERROR_TIME								-6
#define ERROR_NO_FRAG							-7

#define NO_SECOND_FILE							-8
#define NO_SECOND_PATH							""
#define NO_GRACE 								-1
#define _FILE_OFFSET_BITS 						64

#define FALSE									0
#define TRUE									1

#define NUM_OPERATIONS                          7
#define MIN_ARGC								3
#define BLOCK_NUM								3
#define BLOCK_CLOSE_SIZE						(sizeof(char)*strlen(BLOCK_CLOSE_DELIMETER))
#define BLOCK_OPEN_SIZE							(sizeof(char)*strlen(BLOCK_OPEN_DELIMETER))
#define BLOCK_DELIMITER_SIZE 					((BLOCK_OPEN_SIZE) + (BLOCK_CLOSE_SIZE))
#define BLOCK_DELETE_SIZE						(sizeof(char)*strlen(BLOCK_DELETE_DELIMETER))

#define DELIMITER_SIZE	 						8
#define BUFF_SIZE	 							2 * SIZE_KB
#define MAX_FILE_NAME							256
#define MAX_FILE_STORE							100
#define MIN_VAULT_SIZE							sizeof(Catalog)

#define SIZE_INCREASE_UNIT 						1024
#define SIZE_BYTE 								1
#define SIZE_KB 								(SIZE_INCREASE_UNIT * SIZE_BYTE)
#define SIZE_MB 								(SIZE_INCREASE_UNIT * SIZE_KB)
#define SIZE_GB 								(SIZE_INCREASE_UNIT * SIZE_MB)
#define LONG_LONG_MAX							9223372036854775807LL
#define MAX_GIGA_NUM_DIG									6

#define BLOCK_OPEN_DELIMETER								"<<<<<<<<"
#define BLOCK_CLOSE_DELIMETER								">>>>>>>>"
#define CUR_DIR_PATH										"."

#define BYTES 												'B'
#define KILO_BYTES 											'K'
#define MEGA_BYTES 											'M'
#define GIGA_BYTES 											'G'

#define OPERATIONS_STR                          			"init, list, add, rm, fetch, defrag, status"
#define EMPTY_FILE_STR										"file: %s is Empty - cannot copy\n"
#define SIZE_INPUT_STR										"Input: %s - is invalid\nshould be in format: XXXXC\nXXXX - positive long long\nC is from {B, K, M, G}\n"
#define SIZE_INPUT_OVERFLOW_STR								"Input: %s - is overflowing\nmaximum size is %lld Bytes.\n"
#define SIZE_INPUT_UNIT_STR									"Input: %c - isn't a valid unit size choose from to {B, K, M, G}\n"
#define INVALID_ARG_STR 									"Invalid number (%d) of arguments\noperation <%s> accepts %d arguments.\n"
#define INVALID_GENERAL_ARG_STR 							"Invalid number (%d) of arguments - should be at least 2 arguments.\n <VAULT_PATH> <OPERATION_NAME> [depend on operation]\n"
#define INVALID_OP_STR 										"Invalid operation <%s> name.\nChoose operation from {%s}.\n"
#define FILE_SIZE_STR										"Invalid vault size : <%s>\nMinimum vault size: <%s>\n"
#define MEM_ERROR_STR										"Memory allocation failure\n"
#define DELETE_ERROR_STR									"Can't delete %s\nError deleting file: %s\n"
#define OPEN_ERROR_STR										"Can't open %s\nError opening file: %s\n"
#define CLOSE_ERROR_STR										"Can't close %s\nError closing file: %s\n"
#define CHMOD_ERROR_STR										"Error in setting permission to file: %s\nError in chmod with file: %s\n"
#define READ_ERROR_STR										"Can't read %s\nError reading file: %s\n"
#define WRITE_ERROR_STR										"Can't write %s\nError writing file: %s\n"
#define STAT_ERROR_STR										"Can't get stat in %s\nError with stat file: %s\n"
#define SEEK_ERROR_STR										"Error in changing file <%s> seek position\n"
#define TIME_ERROR_STR										"Error in getting time stamp\n"
#define TIME_OFDAY_ERROR_STR								"Error in getting time of day\nError with timeofday function: %s\n"
#define DIR_WRITE_ERROR_STR									"Can't write to current directory.\nError accessing directory: %s\n"
#define VAULT_EXIT_GRACE_STR								"Failure scenario.\nTrying to fix Vault <%s> and exit gracefully.\n"
#define VAULT_GRACE_SUCCESS_STR								"Grace exit success, vault <%s> is consistent try again last operation.\n"
#define VAULT_GRACE_FAIL_STR								"Grace exit Failure, vault <%s> maybe corrupted.\n"
#define VAULT_CREATED_STR									"Result: A vault created\n"
#define VAULT_INSERTED_STR									"Result: %s inserted\n"
#define VAULT_DELETED_STR									"Result: %s deleted\n"
#define VAULT_FETCH_STR										"Result: %s created\n"
#define VAULT_DEFRAG_STR									"Result: Defragmentation complete\n"
#define VAULT_STATUS_STR									"Number of files:\t%hi\nTotal size:\t\t%s\nFragmentation ratio:\t%.1Lf\n"
#define VAULT_FILE_SIZE_STR									"File <%s> is too big <%s>.\nVault free space is <%s>.\n"
#define VAULT_FRAG_STR										"Can't add <%s> to vault - cannot fragment to %d blocks.\nTry defrag operation and repeat...\n"
#define VAULT_FAT_MAX_ERROR_STR								"Vault FAT is full\nMaximum number of files is %d\n"
#define VAULT_DELETED_ERROR_STR								"Failed To delete file <%s> in vault <%s> - Vault maybe not consistent.\nTry operation again check file permissions."
#define FAT_NAME_STR										"File name <%s> is already in Vaults FAT try different name (Case Sensitive).\n"
#define FAT_NAME_NOT_FOUND_STR								"File name <%s> is not in Vaults FAT try different name (Case sensitive).\n"
#define FAT_ENTRY_STR										"%s  \t\t%s\t\t%04o\t\t%s\n"
#define TIME_ELAPSED_STR									"Elapsed time: %.3f milliseconds\n"
#define PARSE_NAME_STR										"Failed To parse file name of path <%s>.\n"

#define TIME_ERROR_MSG() 									printf(TIME_ERROR_STR);
#define TIME_OFDAY_ERROR_MSG() 								printf(TIME_OFDAY_ERROR_STR, strerror( errno ));
#define TIME_ELAPSED_MSG(mt)								printf(TIME_ELAPSED_STR, mt);
#define INVALID_GENERAL_ARGC_MSG(num) 			        	printf(INVALID_GENERAL_ARG_STR, num);
#define INVALID_ARGC_MSG(num,op_name, args) 				printf(INVALID_ARG_STR, num, op_name, args);
#define INVALID_OP_MSG(op_name) 							printf(INVALID_OP_STR, op_name, OPERATIONS_STR);
#define INVALID_SIZE_MSG(inputSize)							printf(SIZE_INPUT_STR, inputSize);
#define INVALID_SIZE_OVERFLOW_MSG(inputSize)				printf(SIZE_INPUT_OVERFLOW_STR, inputSize, LONG_LONG_MAX);
#define INVALID_SIZE_UNIT_MSG(unit)							printf(SIZE_INPUT_UNIT_STR, unit);
#define INVALID_FILE_SIZE_MSG(size, min)					printf(FILE_SIZE_STR, size, min);

#define VAULT_EXIT_GRACE_MSG(path)							printf(VAULT_EXIT_GRACE_STR, path);
#define VAULT_GRACE_SUCCESS_MSG(path)						printf(VAULT_GRACE_SUCCESS_STR,path);
#define VAULT_GRACE_FAIL_MSG(path)							printf(VAULT_GRACE_FAIL_STR,path);
#define VAULT_DELETED_ERROR_MSG(path, file_name);			printf(VAULT_DELETED_ERROR_STR, file_name, path);
#define SEEK_ERROR_MSG(path) 								printf(SEEK_ERROR_STR, path);
#define DELETE_ERROR_MSG(path)								printf(DELETE_ERROR_STR, path, strerror( errno ));
#define OPEN_ERROR_MSG(path)								printf(OPEN_ERROR_STR, path, strerror( errno ));
#define CLOSE_ERROR_MSG(path)								printf(CLOSE_ERROR_STR, path, strerror( errno ));
#define CHMOD_ERROR_MSG(path)								printf(CHMOD_ERROR_STR, path, strerror( errno ));
#define READ_ERROR_MSG(path)								printf(READ_ERROR_STR, path, strerror( errno ));
#define WRITE_ERROR_MSG(path)								printf(WRITE_ERROR_STR, path, strerror( errno ));
#define STAT_ERROR_MSG(path)								printf(STAT_ERROR_STR, path, strerror( errno ));
#define EMPTY_FILE_MSG(path)								printf(EMPTY_FILE_STR, path);
#define DIR_WRITE_ERROR_MSG()								printf(DIR_WRITE_ERROR_STR, strerror( errno ));
#define MEM_ERROR_MSG()										printf(MEM_ERROR_STR);
#define PARSE_NAME_MSG(path)								printf(PARSE_NAME_STR ,path);

#define VAULT_CREATED_MSG()									printf(VAULT_CREATED_STR);
#define VAULT_INSERTED_MSG(file) 							printf(VAULT_INSERTED_STR, file);
#define VAULT_DELETED_MSG(file) 							printf(VAULT_DELETED_STR, file);
#define VAULT_FETCH_MSG(file) 								printf(VAULT_FETCH_STR, file);
#define VAULT_DEFRAG_MSG()		 							printf(VAULT_DEFRAG_STR);
#define VAULT_STATUS_MSG(numOfFiles, sizeStr, ratio)		printf(VAULT_STATUS_STR, numOfFiles, sizeStr, ratio);
#define VAULT_FILE_SIZE_ERROR_MSG(path,size, free_space)	printf(VAULT_FILE_SIZE_STR, path, size, free_space);
#define VAULT_FAT_MAX_ERROR_MSG()							printf(VAULT_FAT_MAX_ERROR_STR, MAX_FILE_STORE);
#define VAULT_FRAG_MSG(file)								printf(VAULT_FRAG_STR, file, BLOCK_NUM);
#define FAT_NAME_MSG(name)									printf(FAT_NAME_STR, name);
#define FAT_NAME_NOT_FOUND_MSG(name)						printf(FAT_NAME_NOT_FOUND_STR, name);
#define FAT_ENTRY_MSG(name,size, per, date)					printf(FAT_ENTRY_STR, name, size, (per&0777), date);

#define MIN(e1, e2)										((e1) < (e2)) ? (e1) : (e2);
#define MIN3(e1, e2, e3)								(e1) < (e2) ? ((e1) < (e3) ? (e1) : (e3)) : ((e2) < (e3) ? (e2) : (e3));
#define MAX(e1, e2)										(e1) > (e2) ? (e1) : (e2);
#define TO_LOWER(str, i)								for(i = 0; str[i]; i++){str[i] = tolower(str[i]);}


#define DELETE_FILE(path)\
		if(remove(path) == ERROR_FILE){\
			DELETE_ERROR_MSG(path);\
			return errno;\
		}

#define CLOSE_FILE(fd, path)\
		if (fd != NO_SECOND_FILE && close(fd) == ERROR_FILE){\
			CLOSE_ERROR_MSG(path);\
			return errno;\
		}

#define CLOSE_FILES(fd1, path1, fd2, path2)\
		CLOSE_FILE(fd1, path1);\
		CLOSE_FILE(fd2, path2);


#define ASSERT_OPEN_SUCCESS(fd, path)\
		if (fd == ERROR_FILE){\
			OPEN_ERROR_MSG(path);\
			return errno;\
		}

#define WRITE_ASSERT_SUCCESS(fdis, buff, write_size, path, fd2, path2)\
		if ((write(fdis, buff, write_size)) != write_size){\
			WRITE_ERROR_MSG(path);\
			CLOSE_FILE(fdis,path) /*try to close*/\
			CLOSE_FILE(fd2, path2);\
			return errno;\
		}

#define READ_ASSERT_SUCCESS(fdis, buff, read_size, path, fd2, path2)\
		if ((read(fdis, buff, read_size)) != read_size){\
			READ_ERROR_MSG(path);\
			CLOSE_FILE(fdis,path) /*try to close*/\
			CLOSE_FILE(fd2, path2);\
			return errno;\
		}


#define SEEK_CHANGE(fd, size, whence, path, fd2, path2) 	\
	if (lseek(fd, size, whence) < 0){\
		SEEK_ERROR_MSG(path);\
		CLOSE_FILES(fd, path,fd2, path2);\
		return ERROR_FILE;	\
	}

#define GET_SEEK_POS(var, fd, path, fd2, path2) 	\
	if ((var = lseek(fd, 0, SEEK_CUR)) < 0){\
		SEEK_ERROR_MSG(path);\
		CLOSE_FILES(fd, path,fd2, path2)\
		return ERROR_FILE;	\
	}

#define INIT_VAULT(md, stat,m_size)\
	md.file_size = 0;\
	md.max_vault_size = m_size;\
	md.creation_time = stat.st_atime;\
	md.modification_time = stat.st_mtime;\
	md.number_files = 0;\
	md.free_space = m_size - sizeof(Catalog);

#define INIT_CATALOG(md, fat, file_name, stat, frags, gaps, current_time)\
	md.number_files++;\
	md.modification_time = fat.insertion_date = current_time;\
	md.free_space -= (status.st_size + (frags*BLOCK_DELIMITER_SIZE));\
	md.file_size += (status.st_size + (frags*BLOCK_DELIMITER_SIZE));\
	fat.is_valid = TRUE;\
	strncpy(fat.name, file_name, MAX_FILE_NAME+1);\
	fat.protection = stat.st_mode;\
	fat.size = stat.st_size;\
	fat.frag_num = frags;\
	fat.block1_offset = gaps[0].start;\
	fat.block2_offset = (frags > 1) ? gaps[1].start : 0;\
	fat.block3_offset = (frags > 2) ? gaps[2].start : 0;\
	fat.block2_size = fat.block3_size = 0;


#define INIT_BINS(b, k, f)\
	b[k].start = f.block1_offset;\
	b[k++].end = f.block1_offset+f.block1_size-1;\
	if(f.block2_size != 0){\
		b[k].start = f.block2_offset;\
		b[k++].end = f.block2_offset+f.block2_size-1;\
		if(f.block3_size != 0){\
			b[k].start = f.block3_offset;\
			b[k++].end = f.block3_offset+f.block3_size-1;\
		}\
	}

#define INIT_DEFRAG(def, k, f, f_index)\
		def[k].fat_index = f_index;\
		def[k].block_index = 1;\
		def[k].bin.start = f.block1_offset;\
		def[k++].bin.end = f.block1_offset+f.block1_size-1;\
		if(f.block2_size != 0){\
			def[k].fat_index = f_index;\
			def[k].block_index = 2;\
			def[k].bin.start = f.block2_offset;\
			def[k++].bin.end = f.block2_offset+f.block2_size-1;\
			if(f.block3_size != 0){\
				def[k].fat_index = f_index;\
				def[k].block_index = 3;\
				def[k].bin.start = f.block3_offset;\
				def[k++].bin.end = f.block3_offset+f.block3_size-1;\
			}\
		}

#define SET_TIME(x, fd1, path1, fd2, path2) \
		if((x = time(NULL)) < 0){\
			TIME_ERROR_MSG();\
			CLOSE_FILES(fd1, path1, fd2, path2);\
			return ERROR_TIME;\
		}

#define DIS_SUM2(h)							dist(h[0])+dist(h[1]);
#define DIS_SUM3(h)							dist(h[0])+dist(h[1])+dist(h[2]);

typedef enum OP {NO_OP, INIT, LIST, ADD, RM, FETCH, DEFRAG, STATUS} OPERATION;

typedef struct bin{
	off_t start; /*block first delimiter byte off set include*/
	off_t end;	/*block last delimiter byte off include include*/

}Bin;

typedef struct frag{
	Bin bin;
	short fat_index;
	char block_index;
}Defrag;


typedef struct args{
	char* vault_name;
	OPERATION operation;
	long long bytesRequested;
	char* path;
}Args;

typedef struct fat{
	char name[MAX_FILE_NAME+1];
	char is_valid;
	short frag_num;
	mode_t protection; /*st_mode*/
	ssize_t size;
	time_t insertion_date;
	ssize_t block1_size; /*including open and close delimiter*/
	ssize_t block2_size; /*including open and close delimiter*/
	ssize_t block3_size; /*including open and close delimiter*/
	off_t block1_offset; /*absolute off set of first delimiter*/
	off_t block2_offset; /*absolute off set of first delimiter*/
	off_t block3_offset; /*absolute off set of first delimiter*/
}Fat;

typedef struct metadata{
	ssize_t file_size;
	ssize_t max_vault_size;
	ssize_t free_space;
	time_t creation_time;
	time_t modification_time;
	short number_files;
} Metadata;

typedef struct catalog{
	Metadata md;
	Fat fat[MAX_FILE_STORE];
} Catalog;

typedef struct vault{
	Catalog catalog;
	char* data;
} Vault;

int getOperationAndArgs(int argc, char** argv, Args* args);
int parse_cli(int argc, char **argv, Args* args);
int parseSize(char* sizeStr, long long* sizeInBytes);
int init(char* vault_path, long long size);
int list(char* vault_path);
int add(char* vault_path, char* file);
int bin_comparator(const void *o1, const void *o2);
short getPartition(ssize_t data_size, long long vault_size, const Bin* bins, int cnt, Bin* res );
int hole_comparator(const void *o1, const void *o2);
int defrag_comparator(const void *o1, const void *o2);
int bin_comparator(const void *o1, const void *o2);
off_t dist(Bin b);
int deleteFatEntry(int fd, char* vault_path, Catalog *catalog, int i);
int rm(char* vault_path, char* file_name);
int fetch(char* vault_path, char* file_name);
int fetchFatEntry(int fdVault, int fdOut, Fat *fat, char* vault_path);
int defrag(char* vault_path);
int deleteBlock(int fd, Bin block, char* vault_path);
off_t* getBlockOffset(Fat* fat, int i);
ssize_t* getBlockSize(Fat* fat, int i);
int status(char* vault_path);
char* bytesToString(long long bytes, char* res, int dots);
int writeBlocks(int fdVault, char* vault_path, int fdIn, char* file_path, Bin* holes, int frag_num, Fat* fat, off_t size);
int gracefullExit(char* vault_path, Bin* blocks, int num);
int zeroAll(int fd, char* vault_path, long long last);

int main(int argc, char **argv) {
	Args args;
	int stat;
	struct timeval start, end;
	long seconds, useconds;
	double mtime;

	/* start time measurement*/
	if(gettimeofday(&start, NULL)< 0){
		TIME_OFDAY_ERROR_MSG();
		return ERROR_TIME;
	}

	stat = parse_cli(argc,argv, &args);
	if(stat == SUCCESS){
		switch (args.operation) {
			case INIT:
				stat = init(args.vault_name, args.bytesRequested);
				break;
			case LIST:
				stat = list(args.vault_name);
				break;
			case ADD:
				stat = add(args.vault_name, args.path);
				break;
			case RM:
				stat = rm(args.vault_name, args.path);
				break;
			case FETCH:
				stat = fetch(args.vault_name, args.path);
				break;
			case DEFRAG:
				stat = defrag(args.vault_name);
				break;
			case STATUS:
				stat = status(args.vault_name);
				break;
			default: /*NO SUCH OPERATION - args.operation == NO_OP*/
				stat = ERROR_INVALID_INPUT;
				break;
			}
		}

	/*end time measurement and print result*/
    if(gettimeofday(&end, NULL) < 0){
   		TIME_OFDAY_ERROR_MSG();
   		return ERROR_TIME;
    }
    seconds  = end.tv_sec  - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;
    mtime = ((seconds) * 1000 + useconds/1000.0);
    TIME_ELAPSED_MSG(mtime);

    if (stat != SUCCESS) /*invalid arguments*/
		return stat;
    return SUCCESS;
}

int init(char* vault_path, long long size){
	int fd, err;
	char sizeString1[MAX_GIGA_NUM_DIG + 2], sizeString2[MAX_GIGA_NUM_DIG + 2];
	struct stat status;
	Catalog catalog = {{0}};

	/*asserts that total file size is sufficent for Vault catalog and blocks delimiters*/
	if(size < MIN_VAULT_SIZE){
		INVALID_FILE_SIZE_MSG(bytesToString(size,sizeString1, 1), bytesToString(MIN_VAULT_SIZE,sizeString2, 1));
		return ERROR_INVALID_INPUT;
	}

	/*creates vault file- delete if exists*/
	fd = open(vault_path,O_CREAT| O_TRUNC| O_RDWR, 0777);
	ASSERT_OPEN_SUCCESS(fd, vault_path)

	/*get vault file stats*/
	if (fstat(fd, &status) < 0){
		STAT_ERROR_MSG(vault_path);
		DELETE_FILE(vault_path);
		return errno;
	}

	if((err = zeroAll(fd, vault_path, size))< 0){
		DELETE_FILE(vault_path);
		return err;
	}

	/*Initializing Vault file*/
	INIT_VAULT(catalog.md, status, size);

	/*writes catalog to file*/

	if (lseek(fd, 0, SEEK_SET) < 0){
		SEEK_ERROR_MSG(vault_path);
		DELETE_FILE(vault_path);
		return ERROR_FILE;
	}
	if (write(fd, &catalog,sizeof(Catalog)) != sizeof(Catalog)){
		WRITE_ERROR_MSG(vault_path);
		DELETE_FILE(vault_path);
		return ERROR_FILE;
	}

	CLOSE_FILE(fd, vault_path);
	VAULT_CREATED_MSG();
	return SUCCESS;
}

int list(char* vault_path){
	int fd, i;
	Fat fat[MAX_FILE_STORE];
	char res[MAX_GIGA_NUM_DIG+2];

	/*Open vault*/
	fd = open(vault_path,O_RDONLY);
	ASSERT_OPEN_SUCCESS(fd, vault_path);

	/*read fat table*/
	SEEK_CHANGE(fd,sizeof(Metadata), SEEK_SET, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
	READ_ASSERT_SUCCESS(fd, fat, sizeof(fat), vault_path, NO_SECOND_FILE, NO_SECOND_PATH)

	/*Close vault*/
	CLOSE_FILE(fd, vault_path)

	/*prints all files in vault*/
	for (i = 0; i < MAX_FILE_STORE; ++i) {
		if(fat[i].is_valid == TRUE){
			FAT_ENTRY_MSG(fat[i].name, bytesToString(fat[i].size, res, 0), fat[i].protection, ctime(&(fat[i].insertion_date)));
		}
	}
	return SUCCESS;
}

int add(char* vault_path, char* file_path){
	char sizeString1[MAX_GIGA_NUM_DIG + 2], sizeString2[MAX_GIGA_NUM_DIG + 2], *file_name;
	int fdVault, fdIn, i, block_num = 0, files_num = 0, entry = -1, err;
	short frag_num;
	Catalog catalog;
	Bin bins[MAX_FILE_STORE*BLOCK_NUM], holes[BLOCK_NUM];
	struct stat status;
	time_t cur_time;

	/*Open Vault*/
	fdVault = open(vault_path, O_RDWR);
	ASSERT_OPEN_SUCCESS(fdVault, vault_path);

	/*Open File to insert to vault*/
	fdIn =  open(file_path, O_RDONLY);
	if(fdIn < 0){
		OPEN_ERROR_MSG(file_path);
		CLOSE_FILE(fdVault, vault_path);
		return errno;
	}

	/*reads vault and if fails closing all open file descriptors and exits with messages*/
	READ_ASSERT_SUCCESS(fdVault,&catalog, sizeof(Catalog), vault_path, fdIn, file_path);

	/*checks if vault has already maximum number of files in it*/
	if(MAX_FILE_STORE <= catalog.md.number_files){
		VAULT_FAT_MAX_ERROR_MSG();
		CLOSE_FILES(fdVault, vault_path, fdIn, file_path);
		return ERROR_INVALID_INPUT;
	}

	/*checks enough space for file in vault*/
	if(fstat(fdIn, &status) < 0){
		STAT_ERROR_MSG(file_path);
		CLOSE_FILES(fdVault, vault_path, fdIn, file_path);
		return errno;
	}

	if(catalog.md.free_space < status.st_size){
		VAULT_FILE_SIZE_ERROR_MSG(file_path, bytesToString(status.st_size, sizeString1, 1), bytesToString(catalog.md.free_space, sizeString2, 1));
		CLOSE_FILES(fdVault, vault_path, fdIn, file_path);
		return ERROR_INVALID_INPUT;
	}
	/*parse file name from full or relative path*/
	if ((file_name = basename(file_path)) == NULL ){
		PARSE_NAME_MSG(file_name);
		CLOSE_FILES(fdVault, vault_path, fdIn, file_path);
		return ERROR_INVALID_INPUT;
	}

	/*Search if file name exists and initializing bin array to find place for new file*/
	for (i = 0; i < MAX_FILE_STORE && files_num < catalog.md.number_files ; ++i) {
		if(catalog.fat[i].is_valid == FALSE){
			entry = (entry == -1) ? i : entry; /*choosing fat entry if not chosen*/
			continue;
		}
		files_num++;
		/*file name exists*/
		if(strcmp(catalog.fat[i].name, file_name) == 0){
			FAT_NAME_MSG(file_name);
			CLOSE_FILES(fdVault, vault_path, fdIn, file_path);
			return ERROR_INVALID_INPUT;
		}

		/*initializing bins to find place for new file*/
		INIT_BINS(bins, block_num, catalog.fat[i]);
	}
	entry = (entry != -1) ? entry : ((files_num == 0) ? 0 : i); /*choosing fat entry if not set*/

	/*sorts bins*/
	qsort(bins, block_num, sizeof(Bin), bin_comparator);

	/*find gaps - fail if can't fragment file to 3 parts*/
	if((frag_num = getPartition(status.st_size, catalog.md.max_vault_size, bins, block_num, holes)) == ERROR_NO_FRAG){
		VAULT_FRAG_MSG(file_path);
		return ERROR_NO_FRAG;
	}

	/*start writing to Vault*/
	SET_TIME(cur_time, fdVault, vault_path, fdIn, file_path);
	INIT_CATALOG(catalog.md, catalog.fat[entry], file_name, status, frag_num, holes,cur_time);

	err = writeBlocks(fdVault, vault_path, fdIn, file_path, holes, frag_num, &catalog.fat[entry], status.st_size);
	if(err != SUCCESS){
		if(gracefullExit(vault_path, holes, frag_num) != SUCCESS){
			VAULT_GRACE_FAIL_MSG(vault_path);
		}
		return ERROR_FILE;
	}

	/*write catalog to vault*/
	SEEK_CHANGE(fdVault,0, SEEK_SET, vault_path,fdIn, file_path);
	WRITE_ASSERT_SUCCESS(fdVault,&catalog, sizeof(Catalog), vault_path, fdIn, file_path);
	/*Closing resources*/
	CLOSE_FILES(fdVault, vault_path, fdIn, file_path);
	VAULT_INSERTED_MSG(file_name);
	return SUCCESS;
}

int rm(char* vault_path, char* file_name){
	int fd,i = 0, files_num = 0, status;
	Catalog catalog;

	/*Open Vault*/
	fd = open(vault_path, O_RDWR);
	ASSERT_OPEN_SUCCESS(fd, vault_path);

	/*reads vault and if fails closing all open file descriptors and exits with messages*/
	READ_ASSERT_SUCCESS(fd,&catalog, sizeof(Catalog), vault_path, NO_SECOND_FILE, NO_SECOND_PATH);

	for (i = 0; i < MAX_FILE_STORE && files_num < catalog.md.number_files ; ++i) {
		if(catalog.fat[i].is_valid == FALSE)
			continue;

		files_num++;
		/*file name exists- start deleting*/
		if(strcmp(catalog.fat[i].name, file_name) == 0){
			if((status = deleteFatEntry(fd,vault_path,&catalog,i)) < 0)
				return status;
			CLOSE_FILE(fd, vault_path);
			VAULT_DELETED_MSG(file_name);
			return SUCCESS;
		}
	}
	/*Didn't find name in vault - closing resources*/
	FAT_NAME_NOT_FOUND_MSG(file_name);
	CLOSE_FILE(fd, vault_path);
	return ERROR_INVALID_INPUT;
}

int fetch(char* vault_path, char* file_name){
	int fdVault, fdOut, i, files_num = 0, status;
	Catalog catalog;

	/*check if current dir is writable*/
	if(access(CUR_DIR_PATH, W_OK) == ERROR_FILE){
		DIR_WRITE_ERROR_MSG();
		return errno;
	}

	/*Open Vault*/
	fdVault = open(vault_path, O_RDWR);
	ASSERT_OPEN_SUCCESS(fdVault, vault_path);

	/*reads vault and if fails closing all open file descriptors and exits with messages*/
	READ_ASSERT_SUCCESS(fdVault,&catalog, sizeof(Catalog), vault_path, NO_SECOND_FILE, NO_SECOND_PATH);

	/*Search if file name exists and initializing bin array to find place for new file*/
	for (i = 0; i < MAX_FILE_STORE && files_num < catalog.md.number_files ; ++i) {
		if(catalog.fat[i].is_valid == TRUE){
			files_num++;
			if(strcmp(catalog.fat[i].name, file_name) == 0){
				/*creating fetch file*/
				if((fdOut = open(catalog.fat[i].name, O_CREAT|O_TRUNC |O_WRONLY, catalog.fat[i].protection)) < 0){
						CLOSE_ERROR_MSG(catalog.fat[i].name);
						CLOSE_FILE(fdVault, vault_path); /*closing vault file*/
						return errno;
				}
				if (chmod(catalog.fat[i].name, catalog.fat[i].protection) < 0){
					CHMOD_ERROR_MSG(catalog.fat[i].name);
					CLOSE_FILES(fdVault, vault_path, fdOut, catalog.fat[i].name); /*closing vault file*/
					return errno;
				}
				/*check if failed fetching file*/
				if ((status = fetchFatEntry(fdVault, fdOut, &catalog.fat[i], vault_path)) < 0 ){
					DELETE_FILE(catalog.fat[i].name);
					return status;
				}

				CLOSE_FILES(fdVault, vault_path, fdOut, catalog.fat[i].name);
				VAULT_FETCH_MSG(catalog.fat[i].name);
				return SUCCESS;
			}
		}
	}

	/*Didn't find file name in fat*/
	FAT_NAME_NOT_FOUND_MSG(file_name);
	CLOSE_FILE(fdVault, vault_path);
	return ERROR_INVALID_INPUT;
}

int defrag(char* vault_path){
	int fd, i, block_num = 0, files_num = 0, bytes, status;
	char buff[BUFF_SIZE];
	off_t read_cur_pos , read_end_pos, write_pos, *block_off;
	Catalog catalog;
	Defrag defrag[MAX_FILE_STORE*BLOCK_NUM];

	/*Open Vault*/
	fd = open(vault_path, O_RDWR);
	ASSERT_OPEN_SUCCESS(fd, vault_path);

	/*reads vault and if fails closing all open file descriptors and exits with messages*/
	READ_ASSERT_SUCCESS(fd,&catalog, sizeof(Catalog), vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
	/*sets write position to start of data block*/
	GET_SEEK_POS(write_pos, fd, vault_path, NO_SECOND_FILE, NO_SECOND_PATH)

	for (i = 0; i < MAX_FILE_STORE && files_num < catalog.md.number_files ; ++i) {
		if(catalog.fat[i].is_valid == TRUE){
			files_num++;
			INIT_DEFRAG(defrag, block_num, catalog.fat[i], i)
		}
	}

	/*sorts defrag bins from smallest offset */
	qsort(defrag, block_num, sizeof(Defrag), defrag_comparator);

	for(i = 0; i < block_num; ++i){
		/*delete the data block*/
		if ((status = deleteBlock(fd, defrag[i].bin, vault_path)) < 0 ){
			return status;
		}

		/*---Write open delimiter blocks in the new position---*/
		block_off = getBlockOffset(&(catalog.fat[defrag[i].fat_index]), defrag[i].block_index);
		*block_off = write_pos;
		SEEK_CHANGE(fd, write_pos, SEEK_SET, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
		WRITE_ASSERT_SUCCESS(fd, BLOCK_OPEN_DELIMETER, BLOCK_OPEN_SIZE, vault_path,  NO_SECOND_FILE, NO_SECOND_PATH);
		GET_SEEK_POS(write_pos, fd, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);/*set write_pos to seek current position*/

		/*---Sets read_position---*/
		read_end_pos = defrag[i].bin.end - BLOCK_CLOSE_SIZE + 1;
		read_cur_pos = defrag[i].bin.start + BLOCK_OPEN_SIZE;

		while(read_cur_pos < read_end_pos){
			/*shouldn't read more than necessary*/
			bytes = MIN(read_end_pos-read_cur_pos, BUFF_SIZE);
			if(bytes == 0) /*done with block*/
				break;

			/*-------Reading data from Vault-----*/
			SEEK_CHANGE(fd, read_cur_pos, SEEK_SET, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
			READ_ASSERT_SUCCESS(fd, buff, bytes, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
			GET_SEEK_POS(read_cur_pos, fd, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
			/*-------Writing data to Vault-----*/
			SEEK_CHANGE(fd, write_pos, SEEK_SET, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
			WRITE_ASSERT_SUCCESS(fd, buff, bytes, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
			GET_SEEK_POS(write_pos, fd, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
		}

		/*write close block delimiter*/
		WRITE_ASSERT_SUCCESS(fd, BLOCK_CLOSE_DELIMETER, BLOCK_CLOSE_SIZE, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
		GET_SEEK_POS(write_pos,fd, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
	}

	/*write catalog to vault*/
	SEEK_CHANGE(fd, 0, SEEK_SET, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
	WRITE_ASSERT_SUCCESS(fd, &catalog, sizeof(Catalog), vault_path, NO_SECOND_FILE, NO_SECOND_PATH);

	/*Closing resources*/
	CLOSE_FILE(fd, vault_path);
	VAULT_DEFRAG_MSG();
	return SUCCESS;
}

int status(char* vault_path){
	int fd, i, files_num = 0, block_num = 0;
	long double consumed_len = 0, sum_gaps = 0, ratio = 0;
	char sizeString[MAX_GIGA_NUM_DIG + 2];
	Catalog catalog;
	Bin blocks[MAX_FILE_STORE*BLOCK_NUM];

	/*Open Vault*/
	fd = open(vault_path, O_RDONLY);
	ASSERT_OPEN_SUCCESS(fd, vault_path);

	/*reads vault and if fails closing all open file descriptors and exits with messages*/
	READ_ASSERT_SUCCESS(fd,&catalog, sizeof(Catalog), vault_path, NO_SECOND_FILE, NO_SECOND_PATH);

	for (i = 0; i < MAX_FILE_STORE && files_num < catalog.md.number_files ; ++i) {
		if(catalog.fat[i].is_valid == TRUE){
			files_num++;
			INIT_BINS(blocks, block_num, catalog.fat[i]);
		}
	}
	/*sorts data block bins from smallest offset */
	qsort(blocks, block_num, sizeof(Bin), bin_comparator);

	if (block_num > 0 ){
		consumed_len = blocks[block_num-1].end - blocks[0].start +1;
		for(i = 1; i < block_num; ++i){
			sum_gaps += blocks[i].start-blocks[i-1].end-1;
		}
	}

	ratio = (consumed_len != 0) ? sum_gaps/consumed_len : 0;
	VAULT_STATUS_MSG(catalog.md.number_files, bytesToString(catalog.md.file_size, sizeString, 1), ratio)
	CLOSE_FILE(fd, vault_path);
	return SUCCESS;
}

/*----------------------AID FUNCTIONS-------------------------------*/

int parse_cli(int argc, char **argv, Args* args){
	int status = 0;

	/*Invalid num of arguments*/
	if (argc < MIN_ARGC){
		INVALID_GENERAL_ARGC_MSG(argc);
		return ERROR_INVALID_INPUT;
	}

	/*default values*/
	args->vault_name = argv[1];
	args->bytesRequested = -1;
	args->operation = NO_OP;
	args->path = NULL;

	status = getOperationAndArgs(argc, argv, args); /*asserts operation name and amount of arguments*/
	if (status < 0)
		return ERROR_INVALID_INPUT;

	return SUCCESS;
}

int getOperationAndArgs(int argc, char** argv, Args* args){
	char* operation = argv[2];
	const char OPS[][NUM_OPERATIONS] = {"init", "list", "add", "rm", "fetch", "defrag", "status"};
	int i;

	TO_LOWER(operation, i);
	args->operation = NO_OP;
	for(i = 0; i < NUM_OPERATIONS; ++i){
	    if(strcmp(OPS[i], operation) == 0){
	        args->operation = i+1; /*including no operation*/
	    }
	}

	switch (args->operation) {
		case NO_OP:
			INVALID_OP_MSG(operation);
			return ERROR_INVALID_INPUT;
		/*4 arguments operations*/
		case INIT:
			if (argc != 4){
				INVALID_ARGC_MSG(argc-2, operation, 1);
				return ERROR_INVALID_INPUT;
			}
			return parseSize(argv[3], &(args->bytesRequested));
		/*4 arguments operations*/
		case ADD:
		case RM:
		case FETCH:
			if (argc != 4){
				INVALID_ARGC_MSG(argc-2, operation, 1);
				return ERROR_INVALID_INPUT;
			}
			args->path = argv[3];
			return SUCCESS;

		default:/*3 arguments operations*/
			if (argc != 3){
				INVALID_ARGC_MSG(argc-2, operation, 0);
				return ERROR_INVALID_INPUT;
			}
			return SUCCESS;
	}
}

/*	Checks if @param sizeStr is of format "XXXXC" where XXXX is positive number till LONG_LONG_MAX
 *  and C is from {B, K, M, G} if so @param sizeInBytes will have value of the string format in bytes.
 *
 *
 * @ret
 *  if fails return negative number and prints ERROR MSG,
 *  Else returns 0.
 */
int parseSize(char* sizeStr, long long* sizeInBytes){
	long long size;
	char unit, *endptr;

	unit = sizeStr[strlen(sizeStr)-1];
	sizeStr[strlen(sizeStr)-1] = '\0';
	size = strtoll(sizeStr, &endptr, 10);

	if (endptr == sizeStr || size <= 0){
		sizeStr[strlen(sizeStr)] = unit;
		INVALID_SIZE_MSG(sizeStr);
		return ERROR_INVALID_INPUT;
	}
	if (size == LONG_LONG_MAX  && errno == ERANGE){
		 INVALID_SIZE_OVERFLOW_MSG(sizeStr);
		 return ERROR_INVALID_INPUT;
	}

	switch (toupper(unit)) {
		case BYTES:
			*sizeInBytes = size * SIZE_BYTE;
			break;
		case KILO_BYTES:
			*sizeInBytes = size * SIZE_KB;
			break;
		case MEGA_BYTES:
			*sizeInBytes = size * SIZE_MB;
			break;
		case GIGA_BYTES:
			*sizeInBytes = size * SIZE_GB;
			break;
		default:
			INVALID_SIZE_UNIT_MSG(unit);
			return ERROR_INVALID_INPUT;

	}
	return SUCCESS;
}

/*@pre bins are sorted*/
short getPartition(ssize_t data_size, long long vault_size, const Bin* bins, int cnt, Bin* res ){
	Bin holes[BLOCK_NUM] = {{0}}, not_needed = {0}, curr;
	char found1 = FALSE, found2 = FALSE, found3 = FALSE;
	int i = 0;

	/*No files*/
	if(cnt == 0){
		res[0].start = sizeof(Catalog);
		res[0].end = vault_size-1;	/*hole at beginning of vault*/
		return ( (dist(res[0])) >= (data_size + BLOCK_DELIMITER_SIZE)) ? 1 : ERROR_NO_FRAG;
	}

	/*Initializing first and last hole*/
	holes[0].start = sizeof(Catalog);
	holes[0].end = bins[0].start-1;
	holes[1].start = bins[cnt-1].end+1;	/*hole at end of vault*/
	holes[1].end = vault_size-1;

	qsort(holes, BLOCK_NUM, sizeof(Bin), hole_comparator); /*sorting from biggest hole to smallest*/
	if ((dist(holes[0])) >= (data_size + BLOCK_DELIMITER_SIZE))
			found1 = TRUE;
	else if ( (dist(holes[0]) + dist(holes[1])) >= (data_size + (2*BLOCK_DELIMITER_SIZE)) && dist(holes[1]) > BLOCK_DELIMITER_SIZE)
			found2 = TRUE;

	for (i = 1; i < cnt; ++i) {
		qsort(holes, BLOCK_NUM, sizeof(Bin), hole_comparator); /*sorting from biggest hole to smallest*/

		/*check if fragmentation found*/
		if ((dist(holes[0])) >= (data_size + BLOCK_DELIMITER_SIZE))
			found1 = TRUE;
		else if ( (dist(holes[0]) + dist(holes[1])) >= (data_size + (2*BLOCK_DELIMITER_SIZE)) && dist(holes[1]) > BLOCK_DELIMITER_SIZE)
			found2 = TRUE;
		else if ((dist(holes[0]) + dist(holes[1]) + dist(holes[2])) >= (data_size + (3*BLOCK_DELIMITER_SIZE)) && dist(holes[2]) > BLOCK_DELIMITER_SIZE)
			found3 = TRUE;

		curr.start = bins[i-1].end+1;
		curr.end = bins[i].start-1;

		/*found 1 block for file*/
		if (found1 == TRUE && dist(curr) >= (data_size+BLOCK_DELIMITER_SIZE) ){
			/*take minimum of hole that file fits*/
			if(dist(curr) < dist(holes[0])){
				holes[0] = curr;
				holes[1] = holes[2] = not_needed; /*not needed*/
			}
		}
		/*found 2 part fragmentation  for file*/
		else if(found2 == TRUE && (dist(curr) + dist(holes[0])) >= (data_size + (BLOCK_DELIMITER_SIZE*2)) ){
			if( dist(curr)  < dist(holes[1]) ){
				holes[1] = curr;
				holes[2] = not_needed; /*not needed*/
			}
		}
		/*found 3 part fragmentation  for file*/
		else if(found3 == TRUE && (dist(curr) + dist(holes[0]) + dist(holes[1])) >= (data_size + (BLOCK_DELIMITER_SIZE*3)) ){
			if( dist(curr)  < dist(holes[2]) ){
				holes[2] = curr;
			}
		}
		/*didn't found 3 part fragmentation for file*/
		else if(dist(curr) > dist(holes[2])){
			holes[2] = curr;
		}
	}
	for(i = 0; i < BLOCK_NUM; ++i) res[i] = holes[i];
	if (found1) return 1;
	if (found2) return 2;
	if (found3) return 3;
	return ERROR_NO_FRAG;
}

int bin_comparator(const void *o1, const void *o2){
    const Bin *b1 = (Bin*)o1;
    const Bin *b2 = (Bin*)o2;
    if (b1->start < b2->start)
    	return -1;
    else
    	return 1;
}

int defrag_comparator(const void *o1, const void *o2){
	const Defrag* d1 = (Defrag*)o1;
	const Defrag* d2 = (Defrag*)o2;
    if (d1->bin.start < d2->bin.start)
    	return -1;
    else
    	return 1;
}

int hole_comparator(const void *o1, const void *o2){
    const Bin *h1 = (Bin*)o1;
    const Bin *h2 = (Bin*)o2;
    if (dist(*h1) < dist(*h2))
    	return 1;
    else if(dist(*h1) == dist(*h2) && h1->start > h2->start)
    	return 1;
    return -1;
}

off_t dist(Bin b){
	return (b.end - b.start +1);
}

/*@pre file descriptor is open for writing*/
int deleteFatEntry(int fd, char* vault_path, Catalog *catalog, int i){
	int status, j = 0;
	Bin blocks[BLOCK_NUM];
	Fat fat = catalog->fat[i];

	ssize_t sum = (fat.block1_size + fat.block2_size + fat.block3_size);
	catalog->md.file_size -= sum;
	catalog->md.free_space += sum;
	catalog->md.number_files--;
	SET_TIME(catalog->md.modification_time, fd, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
	catalog->fat[i].is_valid = FALSE;

	INIT_BINS(blocks, j, catalog->fat[i]);
	SEEK_CHANGE(fd, 0, SEEK_SET, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
	WRITE_ASSERT_SUCCESS(fd,catalog, sizeof(Catalog), vault_path, NO_SECOND_FILE, NO_SECOND_PATH);

	/*deleting all file data blocks*/
	for (j = 0; j < catalog->fat[i].frag_num; ++j) {
		if((status = deleteBlock(fd, blocks[j], vault_path)) < 0 ){
			VAULT_DELETED_ERROR_MSG(vault_path, catalog->fat[i].name);
			return status;
		}
	}
	return SUCCESS;
}

int deleteBlock(int fd, Bin block, char* vault_path){
	char buff[DELIMITER_SIZE]= {0};

	/*change seek to block start position*/
	SEEK_CHANGE(fd, block.start, SEEK_SET, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
	/*write delete block characters instead of open delimiter*/
	WRITE_ASSERT_SUCCESS(fd, buff, sizeof(buff), vault_path, NO_SECOND_FILE, NO_SECOND_PATH)
	/*change seek to block real data end position*/
	SEEK_CHANGE(fd, block.end-BLOCK_CLOSE_SIZE+1, SEEK_SET, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
	/*write delete block characters instead of close delimiter*/
	WRITE_ASSERT_SUCCESS(fd, buff, sizeof(buff), vault_path, NO_SECOND_FILE, NO_SECOND_PATH)
	return SUCCESS;
}

int fetchFatEntry(int fdVault, int fdOut, Fat *fat, char* vault_path){
	int i = 0 , bytes;
	char buff[BUFF_SIZE];
	off_t pos, last;
	Bin off_sets[BLOCK_NUM] = {{0}};

	INIT_BINS(off_sets,i, fat[0]);

	for(i = 0; i < fat->frag_num; ++i){
		/*change seek to the right place*/
		SEEK_CHANGE(fdVault,off_sets[i].start + BLOCK_OPEN_SIZE, SEEK_SET, vault_path, fdOut, fat->name);
		/*set pos to seek current position*/
		GET_SEEK_POS(pos,fdVault, vault_path, fdOut, fat->name);
		last = off_sets[i].end - BLOCK_CLOSE_SIZE + 1;
		while( pos < last ){
			/*shouldn't read more than necessary*/
			bytes = MIN(last-pos, BUFF_SIZE);
			/*-------Reading data from Vault-----*/
			READ_ASSERT_SUCCESS(fdVault, buff, bytes, vault_path, fdOut, fat->name);
				/*-------Writing data to OutPut-----*/
			WRITE_ASSERT_SUCCESS(fdOut, buff, bytes, fat->name, fdVault, vault_path);
			/*update pos to seek current position*/
			GET_SEEK_POS(pos,fdVault, vault_path, fdOut, fat->name);
		}

	}
	return SUCCESS;
}

off_t* getBlockOffset(Fat* fat, int i){
	switch (i) {
				case 1:
					return &(fat->block1_offset);
				case 2:
					return &(fat->block2_offset);
				case 3:
					return &(fat->block3_offset);
				default:
					return NULL;
	}
}

ssize_t* getBlockSize(Fat* fat, int i){
	switch (i) {
		case 1:
			return &(fat->block1_size);
		case 2:
			return &(fat->block2_size);
		case 3:
			return &(fat->block3_size);
		default:
			return NULL;
	}
}

char* bytesToString(long long bytes, char* res, int dots){
	char size = 0; /*maximum file size is XXXXXX GB*/
	double temp = bytes;
	while(size < 3 && temp >= SIZE_INCREASE_UNIT){
		temp /= (double)SIZE_INCREASE_UNIT;
		size++;
	}
	if (dots == 1 && ((int)(temp*10))%10  == 0)
		dots = 0;

	switch (size) {
		case 0:
			if(dots == 1)
				sprintf(res,"%.1lfB",temp);
			else
				sprintf(res,"%.0lfB",temp);
			break;
		case 1:
			if(dots == 1)
				sprintf(res,"%.1lfK",temp);
			else
				sprintf(res,"%.0lfK",temp);
			break;
		case 2:
			if(dots == 1)
				sprintf(res,"%.1lfM",temp);
			else
				sprintf(res,"%.0lfM",temp);
			break;
		case 3:
			if(dots == 1)
				sprintf(res,"%.1lfG",temp);
			else
				sprintf(res,"%.0lfG",temp);
			break;
		default:
			break;
	}
	return res;
}

int gracefullExit(char* vault_path, Bin* blocks, int num){
	int j = 0, status, fd;
	VAULT_EXIT_GRACE_MSG(vault_path);
	/*Open Vault*/
	fd = open(vault_path, O_WRONLY);
	ASSERT_OPEN_SUCCESS(fd, vault_path);
	for (j = 0; j < num; ++j) {
		if((status = deleteBlock(fd, blocks[j], vault_path)) < 0 ){
			return status;
		}
	}
	CLOSE_FILE(fd, vault_path);
	VAULT_GRACE_SUCCESS_MSG(vault_path);
	return SUCCESS;

}


int zeroAll(int fd, char* vault_path, long long last){
	char buff[BUFF_SIZE]= {0};
	off_t pos;
	int bytes;

	GET_SEEK_POS(pos,fd, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
	while( pos <= last ){
		/*shouldn't read more than necessary*/
		bytes = MIN(last-pos, BUFF_SIZE);
		if(bytes == 0)/*done with block*/
			break;
		/*-------Writing data to Vault-----*/
		WRITE_ASSERT_SUCCESS(fd, buff, bytes,vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
		/*set pos to seek current position*/
		GET_SEEK_POS(pos,fd, vault_path, NO_SECOND_FILE, NO_SECOND_PATH);
	}
	return SUCCESS;
}

int writeBlocks(int fdVault, char* vault_path, int fdIn, char* file_path, Bin* holes, int frag_num, Fat* fat, off_t size){
	char buff[BUFF_SIZE];
	int i, bytes;
	off_t pos, last;
	long long total_bytes = 0;
	ssize_t* block_size;

	for(i = 0; i < frag_num; ++i){
		/*change seek to the right place*/
		SEEK_CHANGE(fdVault,holes[i].start, SEEK_SET, vault_path, fdIn, file_path);
		/*write open block delimiter*/
		WRITE_ASSERT_SUCCESS(fdVault, BLOCK_OPEN_DELIMETER, BLOCK_OPEN_SIZE, vault_path, fdIn, file_path);
		/*set pos to seek current position*/
		GET_SEEK_POS(pos,fdVault, vault_path, fdIn, file_path);
		last = holes[i].end - BLOCK_CLOSE_SIZE + 1;

		while( pos < last ){
			/*shouldn't read more than necessary*/
			bytes = MIN3(last-pos, BUFF_SIZE, size-total_bytes);

			if(bytes == 0)/*done with block*/
				break;
			/*-------Reading data from File-----*/
			READ_ASSERT_SUCCESS(fdIn, buff, bytes, file_path, fdVault, vault_path);
			/*-------Writing data to Vault-----*/
			WRITE_ASSERT_SUCCESS(fdVault, buff, bytes,vault_path, fdIn, file_path);
			total_bytes += bytes; /*update total bytes read*/
			/*set pos to seek current position*/
			GET_SEEK_POS(pos,fdVault, vault_path, fdIn, file_path);
		}

		/*write close block delimiter*/
		WRITE_ASSERT_SUCCESS(fdVault, BLOCK_CLOSE_DELIMETER, BLOCK_CLOSE_SIZE,vault_path, fdIn, file_path);
		GET_SEEK_POS(pos,fdVault, vault_path, fdIn, file_path);
		block_size = getBlockSize(fat, i+1);
		*block_size = pos - holes[i].start;
	}
	return SUCCESS;
 }
