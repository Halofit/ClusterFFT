#pragma once

#include <complex>
#include <valarray>

typedef std::complex<double> Complex;

typedef std::valarray<Complex> ComplexArr;
bool operator <(const Complex &a, const Complex &b);

ComplexArr recurFFT(ComplexArr arr);
ComplexArr recurIFFT(ComplexArr arr);

ComplexArr getDominantFrequencies(ComplexArr arr);
ComplexArr shiftArray(ComplexArr arr);
ComplexArr normalise(ComplexArr arr);


ComplexArr getZeros(size_t length);
ComplexArr getRange(int start, size_t length, int stride);

float* writeToFloat(ComplexArr arr);
void printArr(ComplexArr arr);
void printArr(ComplexArr arr, int start, int ammount);
