
#include "ComplexArr.h"

#define _USE_MATH_DEFINES

#include "math.h"



bool operator <(const Complex &a, const Complex &b){
	return a.real() < b.real();
}


ComplexArr getDominantFrequencies(ComplexArr arr){
	ComplexArr retVal;
	retVal = arr*arr;
	return retVal;
}


ComplexArr shiftArray(ComplexArr arr){
	int n = 1000;
	ComplexArr retVal = getZeros(arr.size());
	ComplexArr inter = arr[std::slice(0, arr.size()-n, 1)];
	retVal[std::slice(n, arr.size()-n, 1)] = inter;
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
#undef max()
//nevem tle neki teži
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


ComplexArr generateWave(int freq, size_t size){
	ComplexArr x = getRange(0, size, 1);
	
	x = Complex(2.0*M_PI*freq,0.)*x;

	//x = std::sin(x);
	for (int i = 0; i < x.size(); i++) {
		x[i] = std::sin(x[i]);
	}

	return x;
}