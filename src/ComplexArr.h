#pragma once

#include <complex>
#include <valarray>
#include <vector>

typedef std::complex<float> Complex;

typedef std::valarray<Complex> ComplexArr;
bool operator <(const Complex &a, const Complex &b);

ComplexArr recurFFT(ComplexArr arr);
ComplexArr recurIFFT(ComplexArr arr);

ComplexArr getDominantFrequencies(ComplexArr arr);
ComplexArr shiftArray(ComplexArr arr);
ComplexArr normalise(ComplexArr arr);

std::vector<float> getAmplitude(ComplexArr arr);
std::vector<float> getPhase(ComplexArr arr);

ComplexArr getZeros(size_t length);
ComplexArr getRange(int start, size_t length, int stride);

std::vector<float> writeRealsToFloat(ComplexArr arr);
void printArr(ComplexArr arr);
void printArr(ComplexArr arr, int start, int ammount);
