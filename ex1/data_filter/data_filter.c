#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>


#define SUCCESS 								0
#define FILE_ERROR								-1	/*read system call convention*/
#define SEEK_ERROR								-2
#define MEMORY_FAILURE							-3
#define EMPTY_FILE_ERROR						-4
#define INVALID_INPUT							-5

#define FALSE									0
#define TRUE									1

#define LONG_LONG_MAX							9223372036854775807LL
#define SIZE_INCREASE_UNIT 						1024
#define SIZE_BYTE 								1
#define SIZE_KB 								SIZE_INCREASE_UNIT * SIZE_BYTE
#define SIZE_MB 								SIZE_INCREASE_UNIT * SIZE_KB
#define SIZE_GB 								SIZE_INCREASE_UNIT * SIZE_MB
#define SIZE_SMALL_REQUEST 						SIZE_KB
#define SIZE_BIG_REQUEST 						3 * SIZE_KB

#define PRINT_LOWER_BOUND						32
#define PRINT_UPPER_BOUND						126

#define BYTES 									'B'
#define KILO_BYTES 								'K'
#define MEGA_BYTES 								'M'
#define GIGA_BYTES 								'G'

#define FINAL_REPORT 							"%lld characters requested, %lld characters read, %lld are printable"
#define EMPTY_FILE_STR							"file: %s is Empty - cannot copy\n"
#define SIZE_INPUT_STR							"Input: %s - is invalid\nshould be in format: XXXXC\nXXXX - positive long long\nC is from {B, K, M, G}"
#define SIZE_INPUT_OVERFLOW_STR					"Input: %s - is overflowing\nmaximum size is %lld Bytes.\n"
#define SIZE_INPUT_UNIT_STR						"Input: %c - isn't a valid unit size choose from to {B, K, M, G}\n"
#define INVALID_ARG_STR 						"Invalid number (%d) of arguments - should be exactly 3.\n <DATA_SIZE> <INPUT_DATA_PATH> <OUTPUT_DATA_PATH>"
#define MEM_ERROR_STR							"Memory allocation failure\n"
#define OPEN_ERROR_STR							"Can't open %s\nError opening file: %s\n"
#define CLOSE_ERROR_STR							"Can't close %s\nError closing file: %s\n"
#define READ_ERROR_STR							"Can't read %s\nError reading file: %s\n"
#define WRITE_ERROR_STR							"Can't write %s\nError writing file: %s\n"
#define SEEK_ERROR_STR							"Error in changing seek position\n"

#define SEEK_ERROR_MSG() 								printf(SEEK_ERROR_STR);
#define INVALID_ARG_MSG(num) 							printf(INVALID_ARG_STR, num);
#define INVALID_SIZE_MSG(inputSize)						printf(SIZE_INPUT_STR, inputSize);
#define INVALID_SIZE_OVERFLOW_MSG(inputSize)			printf(SIZE_INPUT_OVERFLOW_STR, inputSize, LONG_LONG_MAX);
#define INVALID_SIZE_UNIT_MSG(unit)						printf(SIZE_INPUT_UNIT_STR, unit);
#define FINAL_REPORT_MSG(requested, read, printed)		printf(FINAL_REPORT, requested, read, printed);
#define OPEN_ERROR_MSG(path)							printf(OPEN_ERROR_STR, path, strerror( errno ));
#define CLOSE_ERROR_MSG(path)							printf(CLOSE_ERROR_STR, path, strerror( errno ));
#define READ_ERROR_MSG(path)							printf(READ_ERROR_STR, path, strerror( errno ));
#define WRITE_ERROR_MSG(path)							printf(WRITE_ERROR_STR, path, strerror( errno ));
#define EMPTY_FILE_MSG(path)							printf(EMPTY_FILE_STR, path);
#define MEM_ERROR_MSG()									printf(MEM_ERROR_STR);


#define FREE_ALL(in, out) 		free(in); free(out);
#define MIN(e1, e2)				(e1) < (e2) ? (e1) : (e2);
#define SEEK_TO_START(fd) 	\
	if (lseek(fd, 0, SEEK_SET) < 0){\
		SEEK_ERROR_MSG();\
		return FILE_ERROR;	\
	}
#define CLOSE_INPUT_FILE(fdInput,inputPath, fdOutput, outputPath)\
		if (close(fdInput) == FILE_ERROR){\
			CLOSE_ERROR_MSG(inputPath);\
			CLOSE_OUTPUT_FILE(fdOutput, outputPath) /*trying to close second file*/\
			return errno;\
		}
#define CLOSE_OUTPUT_FILE(fdOutput, outputPath)\
		if (close(fdOutput) == FILE_ERROR){\
			CLOSE_ERROR_MSG(outputPath);\
			return errno;\
		}

int parseSize(char*, long long*);
int createPrintableFile(int, char*, int , char*, long long, int, long long* , long long* );
ssize_t fillOutBuffer(char*, char*, int, int, int*);
int isEmptyFile(int fd, char *path);


int main(int argc, char **argv) {
	char *sizeStr, *outPath, *inPath;
	int buffSize, status;
	int fdIn, fdOut, tmpErr;
	long long bytesRequested, totalRead = 0, totalPrinted = 0;

	if(argc != 4){ /*user should inPath exactly 3 arguments*/
		INVALID_ARG_MSG(argc-1);
		return INVALID_INPUT;
	}

	/*parsing command line*/
	sizeStr = argv[1];
	inPath = argv[2];
	outPath = argv[3];

	/*parsing size argument*/
	status = parseSize(sizeStr, &bytesRequested);
	if (status != SUCCESS) /*invalid size*/
		return status;

	/*buffer size initialized by the size of the request*/
	if (bytesRequested >= SIZE_MB)
		buffSize = SIZE_BIG_REQUEST;
	else
		buffSize = SIZE_SMALL_REQUEST;

	/*Opening files*/
	fdIn = open(inPath,O_RDONLY);
	if( fdIn < 0 ){ /*error in opening inPath file*/
		OPEN_ERROR_MSG(inPath);
		return errno;
	}

	/*Edge Case - empty file*/
	if ((status = isEmptyFile(fdIn, inPath)) != FALSE)
		return status;

	fdOut = open(outPath, O_WRONLY | O_CREAT | O_TRUNC, 644);
	if( fdOut < 0 ){ /*error in opening outPath file*/
		OPEN_ERROR_MSG(outPath);
		tmpErr = errno;
		if (close(fdIn) == FILE_ERROR){ /*closing inPath file*/
			CLOSE_ERROR_MSG(inPath);
		}
		return tmpErr;
	}

	/*Copying printable characters to outPath file*/
	status = createPrintableFile(fdIn, inPath, fdOut, outPath, bytesRequested, buffSize, &totalRead, &totalPrinted);
	if (status != SUCCESS)
		return status;

	/*printing report*/
	FINAL_REPORT_MSG(bytesRequested, totalRead, totalPrinted);

	/*Closing MACROS - handling errors if occurs*/
	CLOSE_INPUT_FILE(fdIn, inPath, fdOut, outPath);
	CLOSE_OUTPUT_FILE(fdOut, outPath);
	return SUCCESS;
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
		return INVALID_INPUT;
	}
	if (size == LONG_LONG_MAX  && errno == ERANGE){
		 INVALID_SIZE_OVERFLOW_MSG(sizeStr);
		 return INVALID_INPUT;
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
			return INVALID_INPUT;

	}
	return SUCCESS;
}

/*	Copy from @param fdIn file,
 *  to @param fdOut @param bytesReq of bytes filterting characters whos value is less then 32 and more then 126
 *
 * @ret
 *  if fails return negative number and prints ERROR MSG, Else 0
 */
int createPrintableFile(int fdIn, char* inPath, int fdOut, char* outPath, long long bytesReq, int buffSize, long long* pRead, long long* pPrinted){
	int maxBytesToCopy = 0, bytesToRead = 0, readFromInput = 0, leftToCopy = 0;
	long long bytesRead = 0, bytesPrinted = 0;
	ssize_t bytesCurRead = 0, bytesCurPrint = 0;
	char* inBuf = (char*)malloc(sizeof(char)*buffSize);
	char* outBuf = (char*)malloc(sizeof(char)*buffSize);

	if (inBuf == NULL || outBuf == NULL){/*malloc fail check*/
		FREE_ALL(inBuf, outBuf);
		MEM_ERROR_MSG();
		return MEMORY_FAILURE;
	}

	/* --------READ FROM INPUT AND WRITE PRINTABLE CHARACTERS TO OUTPUT---------*/
	while( bytesRead < bytesReq ){

			/*-------Reading data from File-----*/

			/*shouldn't read more than necessary*/
			bytesToRead = MIN((bytesReq-bytesRead), buffSize);
			bytesCurRead = read(fdIn, inBuf,bytesToRead);

			if (bytesCurRead < 0){ /*reading input file error*/
				FREE_ALL(inBuf, outBuf);
				READ_ERROR_MSG(inPath);
				return FILE_ERROR;
			}
			if (bytesCurRead == 0){ /*Moving seek to beginning when it's at EOF while bytes read is smaller than requested*/
				SEEK_TO_START(fdIn);
			}
			bytesRead += bytesCurRead; /*update total bytes read*/

			/*-------Copying printable data from input buffer to output buffer-----*/

			maxBytesToCopy = buffSize-bytesCurPrint; /*copying Min{buff left size, data read}*/
			bytesCurPrint += fillOutBuffer(inBuf, (outBuf+bytesCurPrint), bytesCurRead, maxBytesToCopy, &readFromInput);

			/*-------Writing data from output buffer to output File-----*/
			/*write only if out buff is full or read all data requested*/
			if (bytesCurPrint == buffSize || bytesRead == bytesReq){
				/*writing out buffer to output filer*/
				if( (write(fdOut, outBuf, bytesCurPrint)) != bytesCurPrint){
					FREE_ALL(inBuf, outBuf);
					WRITE_ERROR_MSG(outPath);
					return FILE_ERROR;
				}
				bytesPrinted += bytesCurPrint;
				/*moving not handled data from input buffer to out put buffer*/
				leftToCopy = bytesCurRead-readFromInput;
				bytesCurPrint = fillOutBuffer((inBuf+readFromInput), outBuf, leftToCopy, buffSize, &readFromInput);
			}
	}

	*pRead = bytesRead;
	*pPrinted = bytesPrinted;
	FREE_ALL(inBuf, outBuf);
	return SUCCESS;
}

/*	Checks if @param fd is an empty file.
 *
 * @ret
 *  if file is empty returns 1
 *  if file isn't empty return 0
 *  if fails return negative number and prints ERROR MSG.
 */
int isEmptyFile(int fd, char *path){
	char c;
	int status = read(fd, &c, 1);
	switch (status) {
		case 1: /*file isn't empty*/
			SEEK_TO_START(fd);
			return FALSE;
		case 0:
			EMPTY_FILE_MSG(path);
			return TRUE;
		default:
			READ_ERROR_MSG(path);
			return FILE_ERROR;
	}
}

/*	Copy chars that their value  is 32<=value<=126 from @param inBuf buffer,
 *  to @param outBuf.
 *   copying minimum from inBuf  @param amount or capacity elements.
 *
 * @ret
 *  returns number of characters that were copied and setting @param readFromInput to how many bytes
 *  were read from @param inBuf
 */
ssize_t fillOutBuffer(char* inBuf, char* outBuf, int amount, int capacity, int* readFromInput){
	int i;
	ssize_t cnt = 0;
	/*copying printable chars to out buffer*/
	for (i = 0; i < amount && cnt < capacity; ++i) {
		if ((PRINT_LOWER_BOUND <= inBuf[i]) && (inBuf[i] <= PRINT_UPPER_BOUND))
			outBuf[cnt++] = inBuf[i];
	}
	*readFromInput = i;
	return cnt;
}
