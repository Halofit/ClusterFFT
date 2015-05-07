#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#include <errno.h>
#include <algorithm>

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
//char serverIP[16];
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


int read_line_strings(
	std::vector<std::string>& names,
	std::vector<std::string>& functions,
	std::vector<double>& magnitudes,
	std::string file)
{
	std::ifstream infile(file);
	if (!infile.is_open()){
		return 1;
	}

	std::string line;
	std::string name;
	std::string function;
	double magnitude;

	// Assuming that each line has 1 tokens
	while (std::getline(infile, line)){
		if (line[0] != '#') {
			std::istringstream iss(line);

			if (!(iss >> name)) {
				break; // Error
			}
			// Push to arrays
			names.push_back(name);
		}
		/*
		functions.push_back(function);
		magnitudes.push_back(magnitude);*/
	}
	return 0;
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
	while ((c = fgetc(input)) != '\n') {
		if (c == EOF || c == '\n' || c == '\0') break;

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
int receive_file(SOCKET serverSock, const char* filename, int first, int numFiles, double *startReceive, const char *serverIP){
	
	int nBytes;
	// Prepare buffer
	char buff[BUFFER_SIZE];

	
	// Receive file size - Server sends size of file in first 32 bytes. (Integer)
	char temp[SIZE_FILE_SIZE];
	/*struct timeval tv;

	tv.tv_sec = 300;  /* 30 Secs Timeout 
	tv.tv_usec = 0;
	
	setsockopt(serverSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)); */
	struct timeval selTimeout;
	selTimeout.tv_sec = 2;       /* timeout (secs.) */
	selTimeout.tv_usec = 0;            /* 0 microseconds */
	fd_set readSet;
	/*FD_ZERO(&readSet);
	FD_SET(STDIN_FILENO, &readSet);//stdin manually trigger reading
	FD_SET(socket_desc, &readSet);//tcp socket

	int numReady = select(3, &readSet, NULL, NULL, &selTimeout);*/
	/*FD_SET ReadSet;
	FD_ZERO(&ReadSet);
	FD_SET(serverSock, &ReadSet);
	printf("BEFORE\n");
	nBytes = select(SIZE_FILE_SIZE, &ReadSet, NULL, NULL, NULL);*/
	nBytes = recv(serverSock, temp, SIZE_FILE_SIZE, 0);
	printf("AFTER\n");
	if (nBytes <= 0)
		return socket_error();
	// Accepting first file
	if (first == 0){
		*startReceive = dtime();
		printf("Downloading %d files from server ...\n", numFiles);
	}

	int fileSize = (int)strtol(temp, (char **)NULL, 10);

	printf("Downloading from server (%s) : %s\n", serverIP, filename);
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
		((fileSize - n) < BUFFER_SIZE) ? currBuffSize = fileSize - n : currBuffSize = BUFFER_SIZE;

		// Receive data 
		nBytes = recv(serverSock, buff, currBuffSize, 0);
		if (nBytes > 0){
			// Save bytes to file
			fwrite(buff, sizeof(char), nBytes, f);
			n += nBytes;
			if (LOADER){
				loader(n, fileSize, LOADER_LENGTH);
			}
		}
		else if (nBytes == 0){
			printf("Connection closing...\n");
			break;
		}
		else{
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
int send_file(int serverSock,const char *filename, int row, const char *serverIP){
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
	/*char *name = strrchr(filename, '/');
	if (name == NULL)
		name = filename;
	else
		name++;
	*/
	// Set size for file's name
	char fileNameSize = strlen(filename) + 1;

	// Header - Send size of file name
	nBytes = send(serverSock, &fileNameSize, 1, 0);
	if (nBytes < 0){
		perror("ERROR while sending size of file name.\n");
		return 1;
	}

	// Header - Send file's name
	nBytes = send(serverSock, filename, fileNameSize, 0);
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
	printf("Uploading to server (%s) : %s\n", serverIP, filename);
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
	//FILE *input = stdin;
	std::string input = "stdin";
	std::string function = "speed";
	std::string magnitude = "2.0";

	// Prints working directory
#ifdef _WIN32
	getCWD();
#endif

	// If server's ip not set - error
	if (argc < 2)	{
		printf("\nServer's IP is NOT set.\n");
		exit(1);
	}

	std::string serverIP = argv[1];
	// Get number of files
	if (argc > 2){
		numFiles = atoi(argv[2]);
		if (numFiles > 256){
			printf("Too much files to send.\n");
			exit(1);
		}
	}

	// Read file with files to transfer
	if (argc > 3)
		input = argv[3];
	
	// Set function
	if (argc > 4)
		function = argv[4];
	
	// Set magnitude
	if (argc > 5)
		magnitude = argv[5];
		//magnitude = strdup(argv[5]);
	
	int lineLength = 0;

	// List of files (names)
	std::vector<std::string> listOfFiles;
	std::vector<std::string> functions;
	std::vector<double> magnitudes;
	
	DWORD sockTimeout = 3600 * 1000;


	if (argc < 3)
		printf("\nWrite files to transfer: \n");

	int i = 0;
	// Read lines from STDIN or GIVEN FILE
	if (read_line_strings(listOfFiles, functions,magnitudes,input)){
		printf("Unnable to open '%s': %s\n", input.c_str(), strerror(errno));
	}
	
	

#ifdef _WIN32
	BOOL bOptVal = TRUE;
	int bOptLen = sizeof (BOOL);

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

	

	/* Set the option active 
	bOptVal = TRUE;
	bOptLen = sizeof(bOptVal);
	if (setsockopt(serverSock, SOL_SOCKET, SO_KEEPALIVE, (char *)bOptVal, bOptLen) < 0) {
		perror("setsockopt()");
		close(serverSock);
		exit(EXIT_FAILURE);
	}
	printf("SO_KEEPALIVE set on socket\n");*/

	/*struct timeval tv;

	tv.tv_sec = 300;  // 30 Secs Timeout
	tv.tv_usec = 0;

	setsockopt(serverSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));*/

	// Connect to server
	if (connect(serverSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
		printf("ERROR while connectig to server.\n");
		exit(1);
	}

	iResult = setsockopt(serverSock, SOL_SOCKET, SO_KEEPALIVE, (char *)&bOptVal, bOptLen);
	if (iResult == SOCKET_ERROR) {
		printf("ERROR while setting socket options.\n");
	}


	// Header - Number of files (MAX 256)
	char numFilesChar = numFiles;
	nBytes = send(serverSock, &numFilesChar, 1, 0);
	if (nBytes < 0){
		printf("ERROR while sending number of files.\n");
		exit(1);
	}

	// Argument for number of files bigger than nuber of read files
	if (numFiles > listOfFiles.size()){
		numFiles = listOfFiles.size();
	}
	
	// Send number of files 
	nBytes = send(serverSock, function.c_str(), 20, 0);
	if (nBytes < 0){
		printf("ERROR while sending number of files.\n");
		exit(1);
	}

	// Send magnitude
	nBytes = send(serverSock, magnitude.c_str(), 5, 0);
	if (nBytes < 0){
		printf("ERROR while sending number of files.\n");
		exit(1);
	}

	printf("Uploading %d files to server ...\n", numFiles);
	double startALL = dtime();
	// Send files
	for (i = 0; i < numFiles; i++){
		if (send_file(serverSock, listOfFiles[i].c_str(), i,serverIP.c_str()))
			printf("ERROR while sending %s file. \n", listOfFiles[i]);
	}

	// Prepare outfile names while waiting for server
	for (i = 0; i < numFiles; i++){	
		listOfFiles[i] = "out_" + listOfFiles[i];
	}

	printf("\n");
	printf("Running FFT on server ...\n\n");
	double startReceiving;
	
	// Receive files
	for (i = 0; i < numFiles; i++){
		if (receive_file(serverSock, listOfFiles[i].c_str(), i, numFiles, &startReceiving, serverIP.c_str()))
			printf("ERROR while receiving file! (%s) \n", listOfFiles[i]);
	}
	
	double endReceiving = dtime();
	double endALL = dtime();
	printf("\nReceiving time: %0.4f sec\n", endReceiving - startReceiving);
	printf("Time passed (ALL): %0.4f sec\n", endALL - startALL);

	// Clean up
	listOfFiles.clear();
	return 0;
}
