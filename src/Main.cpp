
#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <mpi.h>

#include <iostream>
#include <cmath>
#include <valarray>
#include <complex>



void initCounter();
void startCounter();
double getCounter();

void mpiWaitForData();

typedef std::complex<double> Complex;
typedef std::valarray<Complex> ComplexArr;

bool operator <(const Complex &a, const Complex &b){
	return a.real() < b.real();
}

bool isPow2(size_t x);
size_t getLog2(size_t x);

ComplexArr modifyArray(ComplexArr arr, int action);

ComplexArr recurFFT(ComplexArr arr);
ComplexArr recurIFFT(ComplexArr arr);
ComplexArr getZeros(size_t length);
ComplexArr getRange(int start, size_t length, int stride);
void printArr(ComplexArr arr);
void printArr(ComplexArr arr, int start, int ammount);
void printArr(float* arr, int start, int ammount);

ComplexArr normalise(ComplexArr arr);
float* writeToFloat(ComplexArr arr);

struct mpiGlobals {
	int procs;
	int procId;
} mpi;

#include "sndfile.h"


int main(int argc, char* argv[]){
	
	float* buf;
	SNDFILE *sf;
	SF_INFO info;
		
	/* Open the WAV file. */
	info.format = 0;
	sf = sf_open("mono.wav", SFM_READ, &info);
	if (sf == NULL){
		printf("Failed to open the file.\n");
		exit(-1);
	}

	size_t num_items = info.frames*info.channels;

	/* Print some of the info, and figure out how much data to read. */
	printf("frames=%d\n", info.frames);
	printf("samplerate=%d\n", info.samplerate);
	printf("channels=%d\n", info.channels);
	printf("num_items=%d\n",num_items);

	/* Allocate space for the data to be read, then read it. */
	buf = (float *) malloc(num_items*sizeof(float));
	size_t num = sf_read_float(sf, buf, num_items);
	sf_close(sf);
	printf("Read %d items\n",num);
	sf_close(sf);

	ComplexArr x;
	size_t totalLen = num_items;
	if(!isPow2(num_items)){
		size_t i;
		for (i = 1; num_items>i; i*=2);
		totalLen = i;
		printf("%u %u\n", totalLen, num_items);
				
	}

	x = ComplexArr(totalLen);
	for (size_t i = 0; i < totalLen; i++) {
		if(i < num_items){
			x[i] = Complex(buf[i], 0.f);
		}else{
			x[i] = Complex(0.f, 0.f);
		}
	}
		
	printf("%u\n", x.size());

	initCounter();
	startCounter();

	ComplexArr x1 = recurFFT(x);
	ComplexArr xInter = modifyArray(x1, 2);
	ComplexArr x2 = recurIFFT(xInter);

	double time = getCounter();

	printArr(x,100,20);
	printArr(normalise(x2),100,20);
	printf("Time: %fms\n", time);

	SNDFILE *outSF;
	SF_INFO oi;
	oi.channels = 1;
	oi.format = (SF_FORMAT_WAV | SF_FORMAT_FLOAT);
	oi.frames = info.frames;
	oi.samplerate = info.samplerate;
	oi.sections = info.sections;
	oi.seekable = info.seekable;

		

	/* Open the WAV file. */
	outSF = sf_open("monoOut.wav", SFM_WRITE, &info);
	if (sf == NULL){
		printf("Failed to open the file.\n");
		exit(-1);
	}


	x2 = normalise(x2);
	float* outBuf = writeToFloat(x2);
	printArr(outBuf,100,20);
	size_t outSize = x2.size();
	if (sf_write_float (outSF, outBuf, num_items) != 1) {
		puts (sf_strerror (outSF));
	}

	sf_close(outSF);
		
	free(buf);
	free(outBuf);
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

ComplexArr modifyArray(ComplexArr arr, int action){
	ComplexArr retVal;
	
	switch(action){
		case 1 :{
			int n = 1000;
			retVal = getZeros(arr.size());
			ComplexArr inter = arr[std::slice(0, arr.size()-n, 1)];
			retVal[std::slice(n, arr.size()-n, 1)] = inter;
		}
		break;

		case 2 :{
			//dobimo dominantne frekvence
			retVal = arr*arr;
		}
		break;

		case 3 :{
			puts("Unimplemented!");
			exit(0);
		}
		break;

		case 0:
		default:
			retVal = arr;
	}

	return retVal;
}


ComplexArr recurFFT(ComplexArr arr){
	size_t n = arr.size();
	if (n == 1){
		return arr;
	}else{
		size_t m = n/2;
		ComplexArr yTop = recurFFT(arr[std::slice(0,m,2)]);
		ComplexArr yBottom = recurFFT(arr[std::slice(1,m,2)]);

		Complex root = std::exp(-2 * M_PI * Complex(0.f,1.f) / double(n));
		
		
		ComplexArr range = getRange(0, m, 1);
		//ComplexArr d = std::pow(root, range);
		for (size_t i = 0; i < m; i++) {
			range[i] = std::pow(root, range[i]);
		}
		ComplexArr d = range;

		ComplexArr z = d*yBottom;
		ComplexArr y(n);
		y[std::slice(0,m,1)] = yTop+z;
		y[std::slice(m,m,1)] = yTop-z;
		return y;
	}
}

ComplexArr recurIFFT(ComplexArr arr){
	size_t n = arr.size();
	if (n == 1){
		return arr;
	}else{
		size_t m = n/2;
		ComplexArr yTop = recurIFFT(arr[std::slice(0,m,2)]);
		ComplexArr yBottom = recurIFFT(arr[std::slice(1,m,2)]);

		Complex root = std::exp(-2 * M_PI * Complex(0.f,1.f) / double(n));
		root = Complex(1.,0.) / root;

		ComplexArr range = getRange(0, m, 1);

		//ComplexArr d = std::pow(root, range);
		for (size_t i = 0; i < m; i++) {
			range[i] = std::pow(root, range[i]);
		}
		ComplexArr d = range;

		ComplexArr z = d*yBottom;
		ComplexArr y(n);
		y[std::slice(0,m,1)] = yTop+z;
		y[std::slice(m,m,1)] = yTop-z;
		return y;
	}
}


ComplexArr getRange(int start, size_t length, int stride){
	ComplexArr r(length);
	int val = start;
	for (size_t i = 0; i < length; i++) {
		r[i] = val;
		val += stride;
	}
	return r;
}

ComplexArr getZeros(size_t length){
	ComplexArr r(length);
	for (size_t i = 0; i < length; i++) {
		r[i] = 0;
	}
	return r;
}


ComplexArr normalise(ComplexArr arr){
#undef max //nevem tle neki teži
	Complex max = arr.max();
	return (arr / max);
}

float* writeToFloat(ComplexArr arr){
	float* retVal = (float*) malloc(sizeof(float)*arr.size());

	for (size_t i = 0; i < arr.size(); i++) {
		retVal[i] = (float)arr[i].real();
	}

	return retVal;
}


void printArr(ComplexArr arr){
	for (int i = 0; i < arr.size(); i++) {
		printf("(%f,%fi) ", arr[i].real(), arr[i].imag());
	}
	
	printf("\n\n");
}

void printArr(ComplexArr arr, int start, int ammount){
	for (int i = start; i < start+ammount && i<arr.size(); i++) {
		printf("(%f,%fi) ", arr[i].real(), arr[i].imag());
	}
	
	printf("\n\n");
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

ComplexArr generateWave(int freq, size_t size){
	ComplexArr x = getRange(0, size, 1);
	
	x = Complex(2.0*M_PI*freq,0.)*x;

	//x = std::sin(x);
	for (int i = 0; i < x.size(); i++) {
		x[i] = std::sin(x[i]);
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