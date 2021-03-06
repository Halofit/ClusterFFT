
#include <stdio.h>
#include <stdlib.h>

#include "gnuplotCall.h"

//Coped from the web, it claims:
// Tested on:
// 1. Visual Studio 2012 on Windows
// 2. Mingw gcc 4.7.1 on Windows
// 3. gcc 4.6.3 on GNU/Linux

// Note that gnuplot binary must be on the path
// and on Windows we need to use the piped version of gnuplot
#ifdef _WIN32
    #define GNUPLOT_NAME "gnuplot --persist"
	#define POPEN _popen
	#define PCLOSE _pclose
#else 
    #define GNUPLOT_NAME "gnuplot"
	#define POPEN popen
	#define PCLOSE pclose
#endif

namespace plot{

	void testGnuplot() {
		FILE *pipe = POPEN(GNUPLOT_NAME, "w");

		if (pipe != NULL) {
			fprintf(pipe, "set term wx\n");         // set the terminal
			fprintf(pipe, "plot '-' with lines\n"); // plot type
			for(int i = 0; i < 10; i++)             // loop over the data [0,...,9]
				fprintf(pipe, "%d\n", i);           // data terminated with \n
			fprintf(pipe, "%s\n", "e");             // termination character
			fflush(pipe);                           // flush the pipe

			PCLOSE(pipe);
		}else{
			printf("Could not open pipe\n"); 
		}
	}


	void drawHistogram(std::vector<float> arr, size_t start, size_t len){
		FILE *pipe = POPEN(GNUPLOT_NAME, "w");

		if (pipe != NULL) {
			size_t end = start + len;

			float max = 0.f;
			float min = 0.f;
			for (size_t i = start; (i < end) && (i < arr.size()); i++) {
				if		(arr[i] > max) max = arr[i];
				else if (arr[i] < min) min = arr[i];
			}

			fprintf(pipe, "set term wx\n");
			fprintf(pipe, "set title 'Hello floats' \n");
			fprintf(pipe, "set style histogram \n");
			fprintf(pipe, "set xrange[0:%d]\n", arr.size()/2);
			fprintf(pipe, "plot [t=0:%u] [%f:%f] '-' with histograms\n", len, min, max);

			for (size_t i = start; (i < end) && (i < arr.size()); i++) {
				fprintf(pipe, "%f\n", arr[i]);
			}

			fprintf(pipe, "%s\n", "e");
			fflush(pipe);

			PCLOSE(pipe);
		}else{
			printf("Could not open pipe\n"); 
		}
	}

	void drawHistogram(std::vector<float> arr, size_t start, size_t len, size_t samplingRateInHz){
		FILE *pipe = POPEN(GNUPLOT_NAME, "w");

		if (pipe != NULL) {
			size_t end = start + len;


			fprintf(pipe, "set term wx\n");
			fprintf(pipe, "set title 'Hello floats' \n");
			fprintf(pipe, "set style histogram \n");
			fprintf(pipe, "plot [t=0:%u] '-' with histograms\n", samplingRateInHz);

			for (size_t i = start; (i < end) && (i < arr.size()); i++) {
				fprintf(pipe, "%f\n", arr[i]);
			}

			fprintf(pipe, "%s\n", "e");
			fflush(pipe);

			PCLOSE(pipe);
		}else{
			printf("Could not open pipe\n"); 
		}
	}


	void drawHistogram(std::vector<float> arr, std::vector<float> freqs, size_t start, size_t len){
		FILE *pipe = POPEN(GNUPLOT_NAME, "w");

		if (pipe != NULL) {
			size_t end = start + len;

			float max = 0.f;
			float min = 0.f;
			for (size_t i = start; (i < end) && (i < arr.size()); i++) {
				if		(arr[i] > max) max = arr[i];
				else if (arr[i] < min) min = arr[i];
			}

			fprintf(pipe, "set term wx\n");
			fprintf(pipe, "set title 'Hello floats' \n");
			fprintf(pipe, "set xrange[0:%f]\n", freqs.at(freqs.size()-1));
			fprintf(pipe, "set yrange[%f:%f]\n", min, max);
			fprintf(pipe, "set style histogram \n");
			//fprintf(pipe, "plot [t=0:%u] [%f:%f] '-' with histograms\n", len, min, max);
			fprintf(pipe, "plot [t=0:%u]  '-' with histograms\n", len);

			for (size_t i = start; (i < end) && (i < arr.size()); i++) {
				fprintf(pipe, "%f\n", arr[i]);
			}

			fprintf(pipe, "%s\n", "e");
			fflush(pipe);

			PCLOSE(pipe);
		}else{
			printf("Could not open pipe\n"); 
		}
	}


	void drawHistogram(std::vector<float> arr){	
		drawHistogram(arr, 0, arr.size());
	}


	void drawLine(std::vector<float> arr, size_t start, size_t len){
		FILE *pipe = POPEN(GNUPLOT_NAME, "w");

		if (pipe != NULL) {
			size_t end = start + len;

			float max = 0.f;
			float min = 0.f;
			for (size_t i = start; (i < end) && (i < arr.size()); i++) {
				if		(arr[i] > max) max = arr[i];
				else if (arr[i] < min) min = arr[i];
			}

			fprintf(pipe, "set term wx\n");
			fprintf(pipe, "set title 'Hello floats' \n");
			fprintf(pipe, "set style line \n");
			fprintf(pipe, "plot [t=0:%u] [%f:%f] '-' with histograms\n", len, min, max);

			for (size_t i = start; (i < end) && (i < arr.size()); i++) {
				fprintf(pipe, "%f\n", arr[i]);
			}

			fprintf(pipe, "%s\n", "e");
			fflush(pipe);

			PCLOSE(pipe);
		}else{
			printf("Could not open pipe\n"); 
		}
	}


	void drawLine(std::vector<float> arr){
		drawLine(arr, 0, arr.size());
	}
}
