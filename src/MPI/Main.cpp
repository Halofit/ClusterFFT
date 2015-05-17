//Mpi version

#define _USE_MATH_DEFINES

#include <string>
#include <iostream>
#include <sstream>
#include <cmath>
#include <valarray>
#include <complex>
#include <map>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <mpi.h>
#ifdef _WIN32
	#include "../Common/sndfile.h"
#else
	#include <sndfile.h>
#endif
#include "../Common/ComplexArr.h"

//#define LOGGING
#ifdef LOGGING
	#define LOG(...) do{if(mpi.rank == mpi.MASTER){printf( __VA_ARGS__); fflush(NULL);}}while(0)
#else
	#define LOG(...)
#endif

void exitMPI(int status){
	MPI_Finalize();
	exit(status);
}

void initCounter();
void startCounter();
double getCounter();

bool isPow2(size_t x);
size_t getLog2(size_t x);
void printArr(float* arr, int start, int ammount);
std::vector<float> getFrequencies(size_t size, size_t sampleRate);

struct MpiGlobals {
	static const int MASTER = 0;
	int size;
	int rank;
} mpi;

struct WavData{
	SF_INFO info;
	std::vector<float> data;
};

WavData loadWavData(const char* file);
bool saveWavData(const char* file, WavData w);

/* // user-defined literal for hertz. Doesn't work
constexpr float operator"" _Hz (float hertz){
    return 1/hertz;
}*/

enum class CmdArgs {
	ARG_FILE_IN, ARG_FILE_OUT, ARG_USE_MPI,
	ARG_USE_FUNCTION, ARG_FUNCTION_MAGNITUDE
};

enum class FFTFun {
	NONE, ARR_SHIFT, POWER, FREQ_SPEED
};

const std::map<std::string, FFTFun> ffrFunctOptions = 
{
	{"none", FFTFun::NONE},
	{"shift", FFTFun::ARR_SHIFT},
	{"power", FFTFun::POWER},
	{"speed", FFTFun::FREQ_SPEED},
};


const std::map<std::string, CmdArgs> options = 
{
	{"-i", CmdArgs::ARG_FILE_IN},
	{"-o", CmdArgs::ARG_FILE_OUT},
	{"-mpi", CmdArgs::ARG_USE_MPI},
	{"-f", CmdArgs::ARG_USE_FUNCTION},
	{"-m", CmdArgs::ARG_FUNCTION_MAGNITUDE},
};

int main(int argc, char* argv[]){
	
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi.size);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi.rank);

	const char* inFile = "kq15.wav";
	const char* outFile = "out.wav";
	FFTFun funct = FFTFun::NONE;
	float functMagn = 1.f;

	//Get arguments into a vector
	std::vector<std::string> cargs(argv, argv + argc);
	
	for (size_t i = 1; i < cargs.size(); i++) {
		auto lookUp = options.find(cargs[i]);
		if(lookUp == options.end()){
			if(mpi.rank == mpi.MASTER) std::cout << "Unknown command " << cargs[i] << "\n";
			exitMPI(3);
		}else{
			switch(options.at(cargs[i])){
				case CmdArgs::ARG_FILE_IN :{
					i++; //go to next arg
					if(i == cargs.size()){
						if(mpi.rank == mpi.MASTER) std::cout << "Error, -i needs a file\n";
						exitMPI(4);
					}
					else{
						inFile = cargs[i].c_str();
					}
					break;
				};

				case CmdArgs::ARG_FILE_OUT :{
					i++; //go to next arg
					if(i == cargs.size()){
						if(mpi.rank == mpi.MASTER) std::cout << "Error, -o needs a file\n";
						exitMPI(4);
					}
					else{
						outFile = cargs[i].c_str();
					}
					break;
				};

				case CmdArgs::ARG_USE_MPI :{
					break;	
				}

				case CmdArgs::ARG_USE_FUNCTION :{
					i++; //go to next arg
					if(i == cargs.size()){
						if(mpi.rank == mpi.MASTER) std::cout << "Error, -f needs a function\n";
						exitMPI(5);
					}
					else{
						auto lookUp = ffrFunctOptions.find(cargs[i]);
						if(lookUp == ffrFunctOptions.end()){
							if(mpi.rank == mpi.MASTER) std::cout << "Unknown function " << cargs[i] << "\n";
							exitMPI(6);
						}else{
							funct = ffrFunctOptions.at(cargs[i]);
						}
					}
					break;
				};
				
				case CmdArgs::ARG_FUNCTION_MAGNITUDE :{
					i++; //go to next arg
					if(i == cargs.size()){
						if(mpi.rank == mpi.MASTER) std::cout << "Error, -m needs a magnitude\n";
						exitMPI(7);
					}
					else{
						std::stringstream ss;
						ss << cargs[i];
						ss >> functMagn;

						if(ss.fail()){
							std::cout << "Error, -m needs valid number.\n";
							exitMPI(8);
						}
					}
					break;
				};
			}
		}
	}

	if(mpi.rank == mpi.MASTER) {
		printf("In:\t%s\nOut:\t%s\nMagnitude: %f\n", inFile, outFile, functMagn);
	}
	
	initCounter();
	startCounter();

	WavData w;
	int* sendcnts = (int*) malloc(sizeoddf(int)*mpi.size);
	int* disps = (int*) malloc(sizeof(int)*mpi.size);

	if(mpi.rank == mpi.MASTER){
		w = loadWavData(inFile);
		
		int disp = 0;
		for (size_t i = 0; i < mpi.size; i++) {
			sendcnts[i] = (int)w.data.size()/mpi.size;
			if(i < (w.data.size()%mpi.size)) sendcnts[i]++;
			disps[i] = disp;
			disp += sendcnts[i];
		}
	}

	
	
	MPI_Bcast(sendcnts, mpi.size, MPI_INT, mpi.MASTER, MPI_COMM_WORLD);
	
	std::vector<float> recvBuf(sendcnts[mpi.rank]);
	
	//Start timer
	MPI_Barrier(MPI_COMM_WORLD);
	double scatterTimeStart = MPI_Wtime();
	
	//Not sure if this will segfault due to first argument
	MPI_Scatterv(w.data.data(), sendcnts, disps,
                MPI_FLOAT, recvBuf.data(), sendcnts[mpi.rank],
                MPI_FLOAT,
                mpi.MASTER, MPI_COMM_WORLD);
	
	//End timer
	MPI_Barrier(MPI_COMM_WORLD);
	double scatterTimeEnd = MPI_Wtime();
	
	
	ComplexArr x;
	size_t totalLen = recvBuf.size();
	//Guarantee that number of elements is a power of 2
	if(!isPow2(recvBuf.size())){
		size_t i;
		for (i = 1; recvBuf.size()>i; i*=2);
		totalLen = i;
	}
	if(mpi.rank == mpi.MASTER) printf("Length of array at %d: %u\n", mpi.rank, totalLen);

	//Pad with zeroes
	x = ComplexArr(totalLen);
	for (size_t i = 0; i < totalLen; i++) {
		if(i < recvBuf.size()){
			x[i] = Complex(recvBuf[i], 0.f);
		}else{
			x[i] = Complex(0.f, 0.f);
		}
	}

	x = recurFFT(x);
	
	switch(funct){
		case(FFTFun::ARR_SHIFT) :{
			x = shiftArray(x, size_t(functMagn));
			x = mirrorArray(x);
		}
		break;

		case(FFTFun::POWER) :{
			x = powArray(x, functMagn);
		}
		break;

		case(FFTFun::FREQ_SPEED) :{
			x = shiftFreqs(x, functMagn);
		}
		break;

		case(FFTFun::NONE):
		default:
			if(mpi.rank == mpi.MASTER) std::cout << "NO FUNCTION! \n";
			break;
	}
	

	x = recurIFFT(x);
	x = normalise(x);

	recvBuf = writeComplexToFloat(x);

	//Start timer
	MPI_Barrier(MPI_COMM_WORLD);
	double gatherTimeStart = MPI_Wtime();
	
	MPI_Gatherv(recvBuf.data(), sendcnts[mpi.rank], MPI_FLOAT,
                w.data.data(), sendcnts, disps,
                MPI_FLOAT, mpi.MASTER, MPI_COMM_WORLD);
	
	
	MPI_Barrier(MPI_COMM_WORLD);
	double gatherTimeEnd = MPI_Wtime();
	
	if(mpi.rank == 0){
		//Prepare data for output
		WavData wOut;
		wOut.info = w.info;
		wOut.data = w.data;
		saveWavData(outFile, wOut);
		double scatterTime = scatterTimeEnd - scatterTimeStart;
		double gatherTime = gatherTimeEnd - gatherTimeStart;
		double time = getCounter();
		printf("\nScatter time: %fs\n", scatterTime);
		printf("Gather time: %fs\n", gatherTime);
		printf("Communication time %fs\n", scatterTime+gatherTime);
		printf("Time: %fms\n\n", time);
	}
	
	MPI_Finalize();
}


WavData loadWavData(const char* file){
	WavData w;
	SNDFILE* sf;
	
	printf("Openeing WAV file [%s]:\n", file);
	w.info.format = 0;
	sf = sf_open(file, SFM_READ, &(w.info));
	if (sf == NULL){
		puts("Failed to open the file.\n");
		exitMPI(-1);
	}
	puts("File opened\n");


	size_t num_items = w.info.frames*w.info.channels;

	/* Print some of the info, and figure out how much data to read. */
	printf("Frames = %d\n", w.info.frames);
	printf("Sample rate = %dHz\n", w.info.samplerate);
	printf("Channels = %d\n", w.info.channels);
	printf("Total number of samples = %d\n", num_items);
	printf("Number of samples per channel = %d\n",num_items/w.info.channels);

	/* Allocate space for the data to be read, then read it. */
	w.data = std::vector<float>(num_items);
	size_t num = sf_read_float(sf, w.data.data(), num_items);
	sf_close(sf);
	printf("Read %d items\n",num);

	return w;
}


bool saveWavData(const char* file, WavData w){
	printf("Opening file [%s] for writing.\n", file);

	SNDFILE *sf = nullptr;
	SF_INFO oi;
	oi.channels = 1;
	oi.format = w.info.format;
	oi.frames = w.info.frames;
	oi.samplerate = w.info.samplerate;
	oi.sections = w.info.sections;
	oi.seekable = w.info.seekable;
		
	// Open the WAV file.
	sf = sf_open(file, SFM_WRITE, &oi);

	if (sf == NULL){
		printf("Failed to open the file.\n");
		return false;
	}

	//Write the file:
	sf_count_t err = sf_write_float (sf, w.data.data(), w.data.size());
	if (err != 1) {
		puts (sf_strerror (sf));
	}
	sf_close(sf);

	return true;
}


std::vector<float> getFrequencies(size_t size, size_t sampleRate){
	size_t n = size;
	size_t nHalf = n/2;

	std::vector<float> ret(nHalf);
	
	for (size_t i = 0; i < nHalf; i++) {
		ret[i] = (i*float(sampleRate))/n;
	}

	return ret;
}


//Yes, I'm taking the piss right now
template <class T>
void printContainer(T arr){
	for (auto& el : arr) {
		std::cout << el << " ";
	}
}

//Furter piss-taking
//Gets a range of elements in any type that supports addition
template <class A>
std::valarray<A> getTemplateRange(A start, size_t length, A stride){
	std::valarray<A> r(length);
	A val = start;
	for (size_t i = 0; i < length; i++) {
		r[i] = val;
		val += stride;
	}
	return r;
}

void printArr(float* arr, int start, int ammount){
	for (int i = start; i < start+ammount; i++) {
		printf("%f ", arr[i]);
	}
	
	printf("\n\n");
}


bool isPow2(size_t x) {
	return x && !(x & (x - 1));
}

size_t getLog2(size_t x){
	switch(x){
		case          1:        return  0;
		case          2:        return  1;
		case          4:        return  2;
		case          8:        return  3;
		case         16:        return  4;
		case         32:        return  5;
		case         64:        return  6;
		case        128:        return  7;
		case        256:        return  8;
		case        512:        return  9;
		case       1024:        return 10;
		case       2048:        return 11;
		case       4096:        return 12;
		case       8192:        return 13;
		case      16384:        return 14;
		case      32768:        return 15;
		case      65536:        return 16;
		case     131072:        return 17;
		case     262144:        return 18;
		case     524288:        return 19;
		case    1048576:        return 20;
		case    2097152:        return 21;
		case    4194304:        return 22;
		case    8388608:        return 23;
		case   16777216:        return 24;
		case   33554432:        return 25;
		case   67108864:        return 26;
		case  134217728:        return 27;
		case  268435456:        return 28;
		case  536870912:        return 29;
		case 1073741824:        return 30;
		default : return 0;
	}
}


#ifdef _WIN32

#include <windows.h>


double pcFreq = 0.0;
__int64 counterStart = 0;

void initCounter(){
	LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li)) exit(0);
	pcFreq = double(li.QuadPart)/1000.0;
}

void startCounter() {
	LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    counterStart = li.QuadPart;
}

double getCounter() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-counterStart)/pcFreq;
}

#else
#include <sys/time.h>

timeval t1;


void initCounter(){
	//The Win32 timer requires this function
}

void startCounter() {
	// start timer
	gettimeofday(&t1, NULL);
}

double getCounter() {
	timeval t2;
	// stop timer
	gettimeofday(&t2, NULL);

	double elapsedTime;
	// compute and print the elapsed time in millisec
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
	return elapsedTime;
}

#endif //_WIN32
