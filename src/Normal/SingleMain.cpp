//Normal version

#define _USE_MATH_DEFINES

#include <iostream>
#include <string>
#include <cmath>
#include <valarray>
#include <complex>
#include <map>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../Common/sndfile.h"
#include "gnuplotCall.h"
#include "../Common/ComplexArr.h"

#define LOGGING
#ifdef LOGGING
	#define LOG(...) do{printf( __VA_ARGS__); fflush(NULL);}while(0)
#else
	#define LOG(...)
#endif

void initCounter();
void startCounter();
double getCounter();

bool isPow2(size_t x);
size_t getLog2(size_t x);
void printArr(float* arr, int start, int ammount);
std::vector<float> getFrequencies(size_t size, size_t sampleRate);

struct WavData{
	SF_INFO info;
	std::vector<float> data;
};

WavData loadWavData(const char* file);
bool saveWavData(const char* file, WavData w);

/* // user-defined literal for hertz. Doesn't work on windows
constexpr float operator"" _Hz (float hertz){
    return 1/hertz;
}*/

enum class CmdArgs {
	ARG_FILE_IN, ARG_FILE_OUT, ARG_USE_MPI, ARG_USE_FUNCTION, ARG_USE_MAGNITUDE,
};

const std::map<std::string, CmdArgs> options = 
{
	{"-i", CmdArgs::ARG_FILE_IN},
	{"-o", CmdArgs::ARG_FILE_OUT},
	{"-mpi", CmdArgs::ARG_USE_MPI},
};

int main(int argc, char* argv[]){
	
	const char* inFile = "kq15.wav";
	const char* outFile = "out.wav";

	//Get arguments into a vector
	std::vector<std::string> cargs(argv, argv + argc);
	for (size_t i = 1; i < cargs.size(); i++) {
		auto lookUp = options.find(cargs[i]);
		if(lookUp == options.end()){
			std::cout << "Unknown command " << cargs[i] << "\n";
			exit(3);
		}else{
			switch(options.at(cargs[i])){
				case CmdArgs::ARG_FILE_IN :{
					i++; //go to next arg
					if(i == cargs.size()){
						std::cout << "Error, -i needs a file\n";
						exit(4);
					}
					else{
						inFile = cargs[i].c_str();
					}
					break;
				};

				case CmdArgs::ARG_FILE_OUT :{
					i++; //go to next arg
					if(i == cargs.size()){
						std::cout << "Error, -o needs a file\n";
						exit(4);
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
						std::cout << "Error, -f needs a function\n";
						exit(5);
					}
					else{
						auto lookUp = ffrFunctOptions.find(cargs[i]);
						if(lookUp == ffrFunctOptions.end()){
							std::cout << "Unknown function " << cargs[i] << "\n";
							exit(6);
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
						exit(7);
					}
					else{
						std::stringstream ss;
						ss << cargs[i];
						ss >> functMagn;

						if(ss.fail()){
							std::cout << "Error, -m needs valid number.\n";
							exit(8);
						}
					}
					break;
				};
			}
		}
	}

	printf("In:\t%s\nOut:\t%s\n", inFile, outFile);
	
	WavData w = loadWavData(inFile);;
	

	ComplexArr x;
	size_t totalLen = w.data.size();
	//Guarantee that number of elements is a power of 2
	if(!isPow2(w.data.size())){
		size_t i;
		for (i = 1; w.data.size()>i; i*=2);
		totalLen = i;
	}
	

	//Pad with zeroes
	x = ComplexArr(totalLen);
	for (size_t i = 0; i < totalLen; i++) {
		if(i < w.data.size()){
			x[i] = Complex(w.data[i], 0.f);
		}else{
			x[i] = Complex(0.f, 0.f);
		}
	}

	initCounter();
	startCounter();

	ComplexArr x1 = recurFFT(x);
	x1 = shiftFreqs(x1, 1.f);
	x = mirrorArray(x);
	ComplexArr x2 = recurIFFT(x1);
	x2 = normalise(x2);

	double time = getCounter();
	printf("Time: %fms\n\n", time);
	
	//Prepare data for output
	WavData wOut;
	wOut.info = w.info;
	wOut.data = writeComplexToFloat(x2);;
	saveWavData(outFile, wOut);
}


WavData loadWavData(const char* file){
	WavData w;
	SNDFILE* sf;
	
	printf("Openeing WAV file [%s]:\n", file);
	w.info.format = 0;
	sf = sf_open(file, SFM_READ, &(w.info));
	if (sf == NULL){
		puts("Failed to open the file.\n");
		exit(-1);
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
