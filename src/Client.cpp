#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#ifdef __linux__ 
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

#include <unistd.h>
#include <arpa/inet.h>

int typedef SOCKET;
double dtime(){
	double tseconds = 0.0;
	struct timeval mytime;

	gettimeofday(&mytime, (struct timezone*) 0);
	tseconds = (double)(mytime.tv_sec + mytime.tv_usec * 1.0e-6);
	return (tseconds);

}
#elif _WIN32
#include <time.h>
#include <direct.h>
#include <io.h>
#include <winsock2.h>
#include <Ws2tcpip.h>

double dtime(){
	double tseconds = (double)clock() / CLOCKS_PER_SEC;
	return (tseconds);
}

void getCWD(){
	char* buffer;

	// Get the current working directory: 
	if ((buffer = _getcwd(NULL, 0)) == NULL)
		perror("_getcwd error");
	else{
		printf("Current working directory: \n%s\n\n", buffer);
		free(buffer);
	}
}
#else
#endif

/*
* errorexit
* ---------
*	Define error function for error output
*	Prints error code, error message and exit
*
*	errorcode - error code
*	errstr - string message
*
*/
#define errorexit(errcode, errstr) \
	fprintf(stderr, "%s: &d\n", errstr, errcode); \
	exit(1);

// GLOBAL variables
char serverIP[16];
int static lastPrinted = 0;

/*
* Defines port
* Buffer size for receiving and sending data
* Number of bites to define size of file - One Integer
* Loader length in command line
* Maximum line length
*/
#define PORT 1053
#define BUFFER_SIZE 4096
#define SIZE_FILE_SIZE 32
#define LOADER_LENGTH 60
#define LINE_LENGTH 100

/*
*   Set 1 to loader if you want to show loader (slower!!!)
*/
#define LOADER 1

/*
* socket_error
* -----------
*	Prints details if error on socket.
*
*	Returns:	1
*/
int socket_error(){
#ifdef _WIN32
	printf("Socket failed with error: %d\n", WSAGetLastError());
#else
	printf("ERROR Socket failure.");
#endif
	return 1;
}


/*
* read_line
* -----------
*	Read line in given input stream.
*	Allocates space for new line, reads line and
*	stores number of read chars to length
*
*	int length		:	to store number of chars
*	char** oldBuff	:	address to allocate memory and store line
*	FILE *input		:	input stream
*
*/
void read_line(int* length, char** oldBuff, FILE *input){
	char* buff = (char*)malloc(sizeof(char)*LINE_LENGTH);
	*oldBuff = buff;
	int count = 0;
	int maxLen = LINE_LENGTH;
	int len = maxLen;
	int c;

	// Read until end of line
	while((c = fgetc(input)) != '\n') {
		if (c == EOF || c == '\n' || c== '\0') break;

		// Char to buffer
		*buff++ = c;
		count++;

		// Reallocate buffer
		if (--len == 0){
			len = maxLen;
			buff = (char *)realloc(oldBuff, maxLen *= 2);
			*oldBuff = buff;
			buff += count;
		}
	}
	*length = count;
	// End of string
	*buff = '\0';
}

/*
* loader
* -----------
*	Shows file uploading or downloading progress
*	in command line
*
*	int val	:	number of bytes already sent
*	int max	:	number of all bytes
*	int size:	loader length in command line
*
*/
void loader(int val, int max, int size){

	int percent = (100.*val) / max;
	if (percent > 100)
		percent = 100;
	int count = ((float)percent * size) / 100;

	if (count == lastPrinted)
		return;
	
	printf("\r[");

	int i;
	for (i = 0; i < count; i++)
		printf("#");
	for (; i < size; i++)
		printf(".");

	printf("] %d%%", percent);
	fflush(stdout);

	lastPrinted = count;
}


/*
* receive_file
* -----------
*	Receive file's information (file size)
*	and receive/write data to file
*
*	int serverSock	:	server socket
*	char *filename	:	filename
*
*	Returns:	1 if ERROR
*				0 if SUCCESS
*/
int receive_file(int serverSock, char* filename){

	int nBytes;
	// Prepare buffer
	char buff[BUFFER_SIZE];

	// Receive file size - Server sends size of file in first 32 bytes. (Integer)
	char temp[SIZE_FILE_SIZE];
	nBytes = recv(serverSock, temp, SIZE_FILE_SIZE, 0);
	if (nBytes <= 0)
		return socket_error();
	int fileSize = (int)strtol(temp, (char **)NULL, 10);

	printf("Downloading from server (%s) : %s\n", serverIP,filename);
	FILE *f = fopen(filename, "wb");

	if (f == NULL){
		printf("ERROR while opening file.\n");
		return 1;
	}

	// Receive file's data
	lastPrinted = 0;
	int n = 0;

	int currBuffSize = BUFFER_SIZE;
	while (n < fileSize){

		// Set buffer size - if buffer size too big receive less bytes (to get bytes of this file not others)
		((fileSize - n) < BUFFER_SIZE) ? currBuffSize = fileSize-n : currBuffSize = BUFFER_SIZE;

		// Receive data 
		nBytes = recv(serverSock, buff, currBuffSize, 0);
		if (nBytes > 0){
			// Save bytes to file
			fwrite(buff, sizeof(char), nBytes, f);
			n += nBytes;
			if (LOADER){
				loader(n, fileSize, LOADER_LENGTH);
			}
		}else if (nBytes == 0){
			printf("Connection closing...\n");
			break;
		}else{
			socket_error();
			break;
		}
	}
	if (LOADER)
		printf("\n");

	// Flush and close file
	fflush(f);
	if (fclose(f) != 0){
		printf("ERROR while closing file.\n");
		return 1;
	}

	return 0;
}

/*
* send_file
* -----------
*	Send file's information (file size, 
*	size of file's name,  file's name)
*	and data to server
*
*	int serverSock	:	server socket
*	char *filename	:	file's name
*	row				:	loader - each file in one row (1. file -> 1. row ...)
*
*	Returns:	1 if ERROR
*				0 if SUCCESS
*/
int send_file(int serverSock, char *filename, int row){
	int nBytes;

	// Open file
	FILE *f = fopen(filename, "rb");

	if (f == NULL){
		printf("ERROR while opening file.\n");
		return 1;
	}

	// Get file's size
	fseek(f, 0, SEEK_END);
	int fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	// Header - Send file's size
	char size[SIZE_FILE_SIZE];
	sprintf(size, "%d", fileSize);
	nBytes = send(serverSock, size, SIZE_FILE_SIZE, 0);
	if (nBytes < 0){
		perror("ERROR while sending size.\n");
		return 1;
	}

	// If absolute path gets only filename
	char *name = strrchr(filename, '/');
	if (name == NULL)
		name = filename;
	else
		name++;

	// Set size for file's name
	char fileNameSize = strlen(name) + 1;

	// Header - Send size of file name
	nBytes = send(serverSock, &fileNameSize, 1, 0);
	if (nBytes < 0){
		perror("ERROR while sending size of file name.\n");
		return 1;
	}

	// Header - Send file's name
	nBytes = send(serverSock, name, fileNameSize, 0);
	if (nBytes < 0){
		perror("ERROR while sending file name.\n");
		return 1;
	}
	//Total bytes written and last printed 
	int n = 0; 
	lastPrinted = 0;

	// Prepare buffer
	char buff[BUFFER_SIZE];

	// Upload file to server
	printf("Uploading to server (%s) : %s\n",serverIP,name);
	fflush(stdout);
	while (!feof(f)){

		// Read bytes to buffer
		int read = fread(buff, 1, BUFFER_SIZE, f);
		if (read < 0){
			printf("ERROR while reading file.\n");
			break;
		}
		// Send bytes from buffer
		nBytes = send(serverSock, buff, read, 0);
		if (nBytes < 0){
			printf("ERROR while sending data.\n");
			break;
		}

		// Show loader GUI
		if (LOADER){
			n += read;
			loader(n, fileSize, LOADER_LENGTH);
		}
	}
	if (LOADER)
		printf("\n");
	fflush(stdout);

	// Close file
	if (fclose(f) != 0){
		printf("ERROR while closing file.\n");
		return 1;
	}

	return 0;
}


/*
* main
* -----------
*	Connects to servers, reads filename from command line or file,
*	sends files to server and receives files.
*
*	argv[1]		:	IP address of server (required)
*	argv[2]		:	Number of files to send (required)
*	argv[3]		:	Specify master file with filenames (optional)
*/
int main(int argc, char *argv[]){

	int serverSock;
	struct sockaddr_in servAddr;
	int nBytes;

	// Default values
	int numFiles = 1;
	FILE *input = stdin;

	// Prints working directory
	#ifdef _WIN32
	getCWD();
	#endif

	// If server's ip not set - error
	if (argc < 2)	{
		printf("\nServer's IP is NOT set.\n");
		exit(1);
	}

	strcpy(serverIP, argv[1]);
	//serverIP = "localhost";
	// Get number of files
	if (argc > 2){
		numFiles = atoi(argv[2]);
		if (numFiles > 256){
			printf("Too much files to send.\n");
			exit(1);
		}
	}

	// Read file with files to transfer
	if (argc > 3){
		input = fopen(argv[3], "r");
		if (input == NULL){
			printf("Unnable to open '%s': %s\n", argv[3], strerror(errno));
			exit(1);
		}
	}

	int lineLength = 0;

	// List of files (names)
	char **listOfFiles = (char **)malloc(numFiles*sizeof(char*));

	// Max length of file name 
	//char *line = (char *)malloc(LINE_LENGTH*sizeof(char));
	int i = 0;
	
	if (argc < 3)
		printf("\nWrite files to transfer: \n");

	// Read lines from STDIN or GIVEN FILE
	for (i; i < numFiles; i++){
		read_line(&lineLength, &listOfFiles[i],input);
	}

	// Close master file
	if (argc > 3)
		fclose(input);

#ifdef _WIN32
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0){
		printf("WSAStartup failed with error: %d\n", iResult);
		exit(1);
	}
#endif

	// Create socket for TCP/IP protocol
	serverSock = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSock < 0){
		errorexit(serverSock, "ERROR opening socket.");
	}

	// Prepare server settings
	memset(&servAddr, '0', sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(PORT);

	// Set server address - Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) <= 0) {
		printf("Inet_pton error occured\n");
		exit(1);
	}

	// Connect to server
	if (connect(serverSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
		printf("ERROR while connectig to server.\n");
		exit(1);
	}

	// Header - Number of files (MAX 256)
	char numFilesChar = numFiles;
	nBytes = send(serverSock, &numFilesChar, 1, 0);
	if (nBytes < 0){
		printf("ERROR while sending number of files.\n");
		exit(1);
	}
	printf("Uploading %d files to server ...\n", numFiles);
	// Send files
	for (i = 0; i < numFiles; i++){
		if (send_file(serverSock, listOfFiles[i], i))
			printf("ERROR while sending %s file. \n", listOfFiles[i]);
	}
	
	// Prepare outfile names while waiting for server
	char* tempFilename;
	for (i = 0; i < numFiles; i++){
		tempFilename = strdup(listOfFiles[i]);
		strncpy(listOfFiles[i] , "out_", sizeof(listOfFiles[i]));
		strncat(listOfFiles[i], tempFilename, sizeof(listOfFiles[i]) - strlen(listOfFiles[i]) - 1);
	}

	printf("\n");
	printf("Downloading %d files from server ...\n", numFiles);
	// Receive files
	for (i = 0; i < numFiles; i++){
		if (receive_file(serverSock, listOfFiles[i]))
			printf("ERROR while receiving file! (%s) \n", listOfFiles[i]);
	}

	// Clean up
	for (i = 0; i < numFiles; i++)
		free(listOfFiles[i]);
	free(listOfFiles);
	free(tempFilename);
	return 0;
}
