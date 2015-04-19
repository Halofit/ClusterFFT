


#define _USE_MATH_DEFINES
#include <cmath>
#include "ComplexArr.h"

#include <complex>
#include <valarray>
#include <vector>
#include <algorithm>

#ifdef _WIN32
bool operator <(const Complex &a, const Complex &b){
	return a.real() < b.real();
}
#endif

Complex getLargestElement(ComplexArr arr){
	Complex retVal;
	for (auto& el : arr) {
		if(el.real() > retVal.real()){
			retVal = el;
		}
	}
	return retVal;
}

ComplexArr squareArray(ComplexArr arr){
	ComplexArr retVal;
	retVal = arr*arr;
	return retVal;
}


ComplexArr shiftArray(ComplexArr arr, size_t n){
	ComplexArr retVal = getZeros(arr.size());
	ComplexArr inter = arr[std::slice(0, arr.size()-n, 1)];
	retVal[std::slice(n, arr.size()-n, 1)] = inter;
	return retVal;
}

ComplexArr shiftFreqs(ComplexArr arr, float constant){
	ComplexArr retVal = getZeros(arr.size());
	for (size_t i = 0; i < (arr.size()/2); i++) {
		retVal[i] = arr[size_t(i/constant)];
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

		Complex root = std::exp(-2 * float(M_PI) * Complex(0.f,1.f) / float(n));
		
		
		ComplexArr range = getRange(0.f, m, 1.f);
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

		Complex root = std::exp(-2 * float(M_PI) * Complex(0.f,1.f) / float(n));
		root = Complex(1.,0.) / root;

		ComplexArr range = getRange(0.f, m, 1.f);

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


std::vector<float> getAmplitude(ComplexArr arr){
	std::vector<float> ret(arr.size());
	for (size_t i = 0; i < arr.size(); i++) {
		float real = arr[i].real();
		float imag = arr[i].imag();
		ret[i] = (float)std::sqrt(real*real + imag*imag);
	}
	return ret;
}


std::vector<float> getPhase(ComplexArr arr){
	std::vector<float> ret(arr.size());
	for (size_t i = 0; i < arr.size(); i++) {
		float real = arr[i].real();
		float imag = arr[i].imag();
		ret[i] = (float)std::atan2(imag, real);
	}
	return ret;
}

std::vector<float> getPowerSpectrum(ComplexArr arr){
	std::vector<float> ret = writeComplexToFloat(std::abs(arr));
	float nsqrd = float(ret.size()*ret.size());
	
	for (size_t i = 0; i < ret.size(); i++) {
		ret[i] = (ret[i]*ret[i])/nsqrd;
	}

	return ret;
}


ComplexArr getRange(float start, size_t length, float stride){
	ComplexArr r(length);
	float val = start;
	for (size_t i = 0; i < length; i++) {
		r[i] = Complex(val, 0.f);
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
	Complex max = getLargestElement(arr);
	return (arr / max);
}

std::vector<float> writeComplexToFloat(ComplexArr arr){
	std::vector<float> retVal (arr.size());

	for (size_t i = 0; i < arr.size(); i++) {
		retVal[i] = arr[i].real();
	}

	return retVal;
}


void printArr(ComplexArr arr){
	for (size_t i = 0; i < arr.size(); i++) {
		printf("(%f,%fi) ", arr[i].real(), arr[i].imag());
	}
	
	printf("\n\n");
}

void printArr(ComplexArr arr, int start, int ammount){
	for (size_t i = start; i < (size_t)start+ammount && i<arr.size(); i++) {
		printf("(%f,%fi) ", arr[i].real(), arr[i].imag());
	}
	
	printf("\n\n");
}


ComplexArr getWave(float freq, size_t size, float sampleRate){
	ComplexArr x = getRange(0.f, size, sampleRate);
	
	x = Complex(2.f*float(M_PI)*freq,0.f)*x;

	//x = std::sin(x);
	for (size_t i = 0; i < x.size(); i++) {
		x[i] = std::sin(x[i]);
	}

	return x;
}


ComplexArr mirrorArray(ComplexArr arr){
	ComplexArr lower = arr[std::slice(0, arr.size()/2, 1)];
	std::reverse(begin(lower), end(lower));

	arr[std::slice(arr.size()/2, arr.size()/2, 1)] = lower;
	return arr;
}
