#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <iostream>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <signal.h>

#ifdef __linux__ 
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>

#define closesocket(sock) (close(sock));

double dtime(){
	double tseconds = 0.0;
	struct timeval mytime;

	gettimeofday(&mytime, (struct timezone*) 0);
	tseconds = (double)(mytime.tv_sec + mytime.tv_usec * 1.0e-6);
	return (tseconds);

}
#elif _WIN32 //Just for testing on windows
#include <time.h>
#include <io.h>
#include <winsock2.h>

double dtime(){
	double tseconds = (double)clock() / CLOCKS_PER_SEC;
	return (tseconds);
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

/*
* Defines port
* Buffer size for receiving and sending data
* Max clients on server.
* Number of bites to define size of file - One Integer
*/
#define PORT 1053
#define BUFFER_SIZE 4096
#define CLIENT_LIMIT 2
#define SIZE_FILE_SIZE 32 
#define TIMEOUT 640
// Global variable for counting clients
int clientCount;
int fileCount;

// Lock for client counter
pthread_mutex_t lockClient = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockFile = PTHREAD_MUTEX_INITIALIZER;

// Struct for client
struct clientInfo{
	int childID;
	int clientSock;
	double runTime;
} typedef clientInfo;


/*
* socket_error
* -----------
*	Prints details if error on socket.
*
*	Returns:	1
*/
int socket_error(){
	printf("ERROR Socket failure.");
	return 1;
}

/*
* server_exit
* -----------
*	Destroys lock and closes listener socket
*
*	int listenerSock	:	listener socket
*/
void server_exit(int listenerSock){
	pthread_mutex_destroy(&lockClient);
	pthread_mutex_destroy(&lockFile);
	pthread_exit(NULL);
	if (listenerSock > 0)
		closesocket(listenerSock);
#ifdef _WIN32
	WSACleanup();
#endif
}


/*
* close_thread
* -----------
*	Substract clientCount, closes client socket
*	and detaches client thread
*
*	int clientSock	:	client socket
*/
void close_thread(int clientSock){
	// Close socket with client 
	closesocket(clientSock);

	// Lock and subtract client to counter
	pthread_mutex_lock(&lockClient);
	clientCount--;
	printf("\nOn server connected %d clients.\n", clientCount);
	pthread_mutex_unlock(&lockClient);

	// Detach thread and release resources
	pthread_detach(pthread_self());
}


/*
* send_file
* -----------
*	Send file's information
*	and data to client
*
*	int clientSock	:	client socket
*	char *filename	:	file's name
*
*	Returns:	1 if ERROR
*				0 if SUCCESS
*/
int send_file(int clientSock, char *filename){

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


	nBytes = send(clientSock, size, SIZE_FILE_SIZE, 0);
	if (nBytes <= 0)
		return socket_error();

	// If absolute path gets only filename
	char *name = strrchr(filename, '/');
	if (name == NULL)
		name = filename;
	else
		name++;

	// Prepare buffer
	char buff[BUFFER_SIZE];

	printf("Sending... : %s (%d Bytes) \n", name, fileSize);
	fflush(stdout);

	int read = 0;
	while (!feof(f)){
		//		printf("FREAD\n");
		// Read bytes to buffer
		read = fread(buff, 1, BUFFER_SIZE, f);
		if (read <= 0){
			printf("ERROR while reading file. \n");
			break;
		}

		// Send bytes from buffer
		nBytes = send(clientSock, buff, read, 0);
		if (nBytes <= 0){
			socket_error();
			break;
		}
		else if (nBytes < read){
			printf("ERROR while sending file.\n");
			break;
		}
	}

	// Close file
	if (fclose(f) != 0){
		printf("ERROR while closing file.\n");
		return 1;
	}
	return 0;
}


/*
* receive_file
* -----------
*	Receive file's information
*	and receive/write data to file
*
*	int clientSock	:	client socket
*	char **listofFiles	:	array to save file's names
*
*	Returns:	1 if ERROR
*				0 if SUCCESS
*/
int receive_file(int clientSock, char **listOfFiles, char **listOfFilesOUT){
	int nBytes;

	// Creates buffer
	char buff[BUFFER_SIZE];

	// Receive file size - Client sends size of file in first 32 bytes. (Integer)
	char temp[SIZE_FILE_SIZE];
	nBytes = recv(clientSock, temp, SIZE_FILE_SIZE, 0);
	if (nBytes < 0)
		return socket_error();
	int fileSize = (int)strtol(temp, (char **)NULL, 10);

	// Receive filename size - Client sents size of filename in one char (1 Byte)
	char nameSizeChar;
	nBytes = recv(clientSock, &nameSizeChar, 1, 0);
	if (nBytes < 0)
		return socket_error();
	int fileNameSize = (int)nameSizeChar;

	// Receive filename 
	char *filename = (char *)malloc((int)fileNameSize + 20);
	nBytes = recv(clientSock, filename, fileNameSize, 0);
	if (nBytes < 0)
		return socket_error();
	char* tempFilename = strdup(filename);

	printf("Receiving... : %s (%d Bytes)\n", filename, fileSize);

	// Lock and get current file ID
	char tempCount[85];
	pthread_mutex_lock(&lockFile);
	sprintf(tempCount, "%d", fileCount);
	fileCount++;
	pthread_mutex_unlock(&lockFile);

	// Set name for saving file
	const char *tempChar = "_";
	strncat(tempCount, tempChar, sizeof(tempCount)-strlen(tempCount) - 1);
	strncat(tempCount, tempFilename, sizeof(tempCount)-strlen(tempCount) - 1);
	*listOfFiles = strdup(tempCount);
	// Prepare FFT outfile name
	memset(&filename[0], 0, fileNameSize);
	strncpy(filename, "out_", sizeof(filename));
	strncat(filename, tempCount, sizeof(filename)-strlen(filename) - 1);
	*listOfFilesOUT = strdup(filename);

	// Open file
	FILE *f = fopen(tempCount, "wb");
	if (f == NULL){
		printf("ERROR while opening file.\n");
		return 1;
	}

	// Receive file's data
	int n = 0;
	int currBuffSize = BUFFER_SIZE;
	while (fileSize > 0){
		// Set buffer size - if buffer size too big receive less bytes (to get only bytes of current file not others)
		(fileSize < BUFFER_SIZE) ? currBuffSize = fileSize : currBuffSize = BUFFER_SIZE;
		// Receive data 
		nBytes = recv(clientSock, buff, currBuffSize, 0);
		if (nBytes > 0){
			// Save bytes to file
			fwrite(buff, sizeof(char), nBytes, f);
			fileSize -= nBytes;
		}
		else if (nBytes == 0){
			printf("Connection closing ...\n");
			break;
		}
		else{
			socket_error();
			break;
		}
	}

	// Flush and close file
	fflush(f);
	if (fclose(f) != 0){
		printf("ERROR while closing file.\n");
		return 1;
	}

	// Clean up
	free(tempFilename);
	free(filename);
	return 0;
}

/*
* client_handle
* -----------------------
*	Handles a client:
*	Add to client count - With lock
*	Receives and sends data
*	When client breaks connection, thread deatach itself.
*	Subtract to client count - With lock
*	Resources are released upon exiting
*
*	s: pointer to clientInfo
*
*	Returns: NULL
*/
void *client_handle(void *s){

	// Lock and add client to counter
	pthread_mutex_lock(&lockClient);
	clientCount++;
	printf("\nOn server connected %d clients.\n", clientCount);
	pthread_mutex_unlock(&lockClient);

	// Get client SOCKET
	clientInfo *c = (clientInfo*)s;
	int nBytes;
	int clientSock = (int)c->clientSock;
	double startTime = (double)c->runTime;

	char fun[20];
	std::string function;
	function.reserve(20);

	char mag[5];
	std::string magnitude;
	magnitude.reserve(5);

	char nrpis;
	std::string rpis;
	rpis.reserve(1);

	// Receive number of files or end client session
	char numFilesChar;
	nBytes = recv(clientSock, &numFilesChar, sizeof(char), 0);
	if (nBytes < 0){
		socket_error();
		close_thread(clientSock);
		return NULL;
	}

	// Receive name of function
	nBytes = recv(clientSock, fun, 20, 0);
	if (nBytes < 0){
		socket_error();
		close_thread(clientSock);
		return NULL;
	}

	// Receive magnitude
	nBytes = recv(clientSock, mag, 5, 0);
	if (nBytes < 0){
		socket_error();
		close_thread(clientSock);
		return NULL;
	}

	// Receive number of RPIs
	nBytes = recv(clientSock, &nrpis, sizeof(char), 0);
	if (nBytes < 0){
		socket_error();
		close_thread(clientSock);
		return NULL;
	}

	function = fun;
	magnitude = mag;
	rpis = nrpis;

	int numFiles = (int)numFilesChar;
	// List of file names
	char **listOfFiles = (char **)malloc(numFiles*sizeof(char*));
	char **listOfFilesOUT = (char **)malloc(numFiles*sizeof(char*));
	printf("\nServer receiving %d files ... \n", numFiles);

	// Receive all files
	int i = 0;
	double startReceiving = dtime();
	for (i; i < numFiles; i++){
		if (receive_file(clientSock, &listOfFiles[i], &listOfFilesOUT[i])){
			printf("ERROR while receiving file! \n");
		}
	}
	double endReceiving = dtime();
	printf("Receiving Time: %0.4f sec\n", (endReceiving - startReceiving));
	printf("\n=============================== FFT OUTPUT =============================== \n");
	// DO SOME HARD WORK - FFT
	//int result;
	double startFFT = dtime();
	int stat;
	pid_t pid;
	char temp;
	int wpid;
	for (i = 0; i < numFiles; i++){
		std::string command = "mpiexec -machinefile machinefile -n ";
		command += rpis;
		command += " ~/RunFFT/clusterFFT.out -f ";
		command += function;
		command += " -m ";
		command += magnitude;
		command += " -i ";
		command += listOfFiles[i];
		command += " -o ";
		command += listOfFilesOUT[i];
		//result = system(command.c_str());
		//const char *cmd = command.c_str();
		if ((pid = fork()) == 0) {
			execl("/bin/sh", "sh", "-c", command.c_str(), (char *)0);
			_exit(127);
		}
		if (pid == -1) {
			stat = -1; /* errno comes from fork() */
		}
		else {
			temp = '0';
			int waittime = 0;
			int Stat;
			do {
				wpid = waitpid(pid, &stat, WNOHANG);
				if (wpid == 0) {
					if (waittime < TIMEOUT) {
						sleep(1);
						waittime++;
						// Send ACK
						send(clientSock, &temp, 1, 0);
					}
					else {
						// Timeout
						printf("Killing child process\n");
						kill(pid, SIGKILL);
					}
				}
			} while (wpid == 0 && waittime <= TIMEOUT);
		}
	}
	if (WIFSIGNALED(stat) && (WTERMSIG(stat) == SIGINT || WTERMSIG(stat) == SIGQUIT)){
		printf("ERROR System returned: %d", stat);
		exit(1);
	}
	temp = '1';
	send(clientSock, &temp, 1, 0);
	double endFFT = dtime();
	printf("\n========================================================================== \n");


	// Send all files back
	for (i = 0; i < numFiles; i++){
		if (send_file(clientSock, listOfFilesOUT[i]))
			printf("ERROR while sending file! (%s)\n", listOfFilesOUT[i]);
	}

	printf("\nRemoving files ...\n");
	// Remove files on server
	for (i = 0; i < numFiles; i++){
		if (remove(listOfFiles[i]))
			printf("ERROR while removing file! (%s)\n", listOfFiles[i]);
		if (remove(listOfFilesOUT[i]))
			printf("ERROR while removing output file! (%s)\n", listOfFilesOUT[i]);
	}

	nBytes = recv(clientSock, fun, 1, 0);
	if (nBytes == 0){
		printf("Connection closed.\n");
	}

	double endTime = dtime();
	printf("\n====================== RESULTS ======================\n");
	printf("FFT Time: %0.4f sec\n", (endFFT - startFFT));
	printf("Time of sending/receiving: %0.4f sec\n", ((endTime - startTime) - (endFFT - startFFT)));
	printf("Time passed (ALL): %0.4f sec\n", (endTime - startTime));
	printf("=====================================================\n");

	// Clean up
	for (i = 0; i < numFiles; i++){
		free(listOfFiles[i]);
		free(listOfFilesOUT[i]);
	}
	free(listOfFiles);
	free(listOfFilesOUT);

	// Close thread
	close_thread(clientSock);
	return NULL;
};


int main(int argc, char **argv){

	// Output status
	int errcode;

	// File counter
	fileCount = 0;

	// Socket and client listener
	int listenerSock;
	int clientSock;

	// Prepare server addres and client address
	struct sockaddr_in servAddr, cliAddr;

	// Library for socket managing (WINDOWS)
#ifdef _WIN32
	int iResult;
	int clilen;
	WSADATA wsaData;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0){
		printf("WSAStartup failed with error: %d\n", iResult);
		exit(1);
	}
#else
	socklen_t clilen;
#endif

	// Open socket
	listenerSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenerSock < 0){
		server_exit(listenerSock);
		errorexit(listenerSock, "ERROR opening socket.");
	}

	int optval = 1;
	socklen_t optlen = sizeof(optval);
	if (setsockopt(listenerSock, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0){
		server_exit(listenerSock);
		errorexit(listenerSock, "ERROR on binding.");
	}

	// Initialize socket structure
	memset(&servAddr, '0', sizeof(servAddr));

	// Set server config
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
	servAddr.sin_port = htons(PORT);

	// Bind socket with port
	if (bind(listenerSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
		server_exit(listenerSock);
		errorexit(listenerSock, "ERROR on binding.");
	}



	// Starts listening
	listen(listenerSock, CLIENT_LIMIT);
	clilen = sizeof(cliAddr);
	printf("Starts listening....\n");
	while (1){


		// Create connection
		clientSock = accept(listenerSock, (struct sockaddr *)&cliAddr, &clilen);
		double runTime = dtime();

		if (clientSock < 0){
			server_exit(listenerSock);
			errorexit(clientSock, "ERROR on accept client.");
		}


		if (setsockopt(clientSock, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0){
			server_exit(listenerSock);
			errorexit(clientSock, "ERROR on set client.");
		}

		// Creates new client thread and arguments
		pthread_t clientThread;
		clientInfo args;
		args.clientSock = clientSock;
		args.runTime = runTime;

		/*
		*  Lock (reading client_count) and check if CLIENT_LIMIT is exceeded
		*  True: Breaks connection
		*  False: Preoceed with client
		*/
		pthread_mutex_lock(&lockClient);
		args.childID = clientCount;
		if ((clientCount + 1) > CLIENT_LIMIT){
			closesocket(clientSock);
			pthread_mutex_unlock(&lockClient);
			continue;
		}

		pthread_mutex_unlock(&lockClient);

		// Create thread and serve client 
		if (errcode = pthread_create(&clientThread, NULL, client_handle, &args)){
			server_exit(listenerSock);
			errorexit(errcode, "pthread_create");
		};
	}

	// Cleans up socket and release resources
	server_exit(listenerSock);
	return 0;
}
