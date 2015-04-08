
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

	int testGnuplot() {
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
    
		return 0;
	}


	void drawHistogram(std::vector<float> arr){	
		FILE *pipe = POPEN(GNUPLOT_NAME, "w");

		if (pipe != NULL) {

			float max = 0.f;
			float min = 0.f;
			for (auto& el : arr) {
				if		(el > max) max = el;
				else if (el < min) min = el;
			}

			fprintf(pipe, "set term wx\n");
			fprintf(pipe, "set title 'Hello floats' \n");
			fprintf(pipe, "set style histogram \n");
			fprintf(pipe, "plot [t=0:%u] [%f:%f] '-' with histograms\n", arr.size(), min, max);

			for (auto& el : arr) {
				fprintf(pipe, "%f\n", el);
			}

			fprintf(pipe, "%s\n", "e");
			fflush(pipe);

			PCLOSE(pipe);
		}else{
			printf("Could not open pipe\n"); 
		}
	}
}
