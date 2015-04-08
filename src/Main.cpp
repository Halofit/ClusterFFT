
#define _USE_MATH_DEFINES

#include <iostream>
#include <cmath>
#include <valarray>
#include <complex>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <mpi.h>

#include "sndfile.h"
#include "gnuplotCall.h"
#include "ComplexArr.h"

void initCounter();
void startCounter();
double getCounter();

void mpiWaitForData();


bool isPow2(size_t x);
size_t getLog2(size_t x);
void printArr(float* arr, int start, int ammount);


int main(int argc, char* argv[]){
	SNDFILE* sf;
	SF_INFO info;
		

	puts("Openeing WAV file:\n");
	info.format = 0;
	sf = sf_open("mono.wav", SFM_READ, &info);
	if (sf == NULL){
		puts("Failed to open the file.\n");
		exit(-1);
	}
	puts("File opened\n");


	size_t num_items = info.frames*info.channels;

	/* Print some of the info, and figure out how much data to read. */
	printf("Frames = %d\n", info.frames);
	printf("Sample rate = %dHz\n", info.samplerate);
	printf("Channels = %d\n", info.channels);
	printf("Total number of samples = %d\n",num_items);
	printf("Number of samples per channel = %d\n",num_items/info.channels);

	/* Allocate space for the data to be read, then read it. */
	std::vector<float> buf(num_items);
	size_t num = sf_read_float(sf, &(buf[0]), num_items);
	sf_close(sf);
	printf("Read %d items\n",num);
	

	ComplexArr x;
	size_t totalLen = num_items;
	//Guarantee that number of elements is a power of 2
	if(!isPow2(num_items)){
		size_t i;
		for (i = 1; num_items>i; i*=2);
		totalLen = i;
	}
	printf("Length of array: %u\n", totalLen);


	//Pad with zeroes
	x = ComplexArr(totalLen);
	for (size_t i = 0; i < totalLen; i++) {
		if(i < num_items){
			x[i] = Complex(buf[i], 0.f);
		}else{
			x[i] = Complex(0.f, 0.f);
		}
	}

	initCounter();
	startCounter();

	ComplexArr x1 = recurFFT(x);
	plot::drawHistogram(getAmplitude(x1));
	x1 = squareArray(x1);
	ComplexArr x2 = recurIFFT(x1);
	x2 = normalise(x2);

	double time = getCounter();
	printf("Time: %fms\n\n", time);

	
	puts("Opening file for writing output\n");
	SNDFILE *outSF = nullptr;
	SF_INFO oi;
	oi.channels = 1;
	oi.format = info.format;
	oi.frames = info.frames;
	oi.samplerate = info.samplerate;
	oi.sections = info.sections;
	oi.seekable = info.seekable;
		
	// Open the WAV file.
	outSF = sf_open("monoOut.wav", SFM_WRITE, &oi);

	if (sf == NULL){
		printf("Failed to open the file.\n");
		exit(-1);
	}else{
		//Prepare data for output
		std::vector<float> outBuf = writeRealsToFloat(x2);
	
		//Write the file:
		size_t outSize = x2.size();
		sf_count_t err = sf_write_float (outSF, &(outBuf[0]), num_items);
		if (err != 1) {
			puts (sf_strerror (outSF));
		}
		sf_close(outSF);
	}
}

void mpiWaitForData(){}

/* MPI garbage zaenkrat leti ven dokler ne zrihtam ostalega
void mpiWaitForData(){
	int signal;
	MPI_Recv(&signal, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

}
{
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi.procs);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi.procId);

	MPI_Finalize();
}
*/


//Yes, I'm taking the piss right now
template <class T>
void printContainer(T arr){
	for (auto& el : arr) {
		std::cout << el << " ";
	}
}

//Furter piss-taking
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
