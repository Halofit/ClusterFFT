#pragma once

#include <complex>
typedef std::complex<float> Complex;
bool operator<(const Complex &a, const Complex &b);

#include <valarray>
#include <vector>



typedef std::valarray<Complex> ComplexArr;


ComplexArr recurFFT(ComplexArr arr);
ComplexArr recurIFFT(ComplexArr arr);

ComplexArr squareArray(ComplexArr arr);
ComplexArr shiftArray(ComplexArr arr, size_t n);
ComplexArr shiftFreqs(ComplexArr arr, float constant);
ComplexArr normalise(ComplexArr arr);
ComplexArr mirrorArray(ComplexArr arr);

std::vector<float> getAmplitude(ComplexArr arr);
std::vector<float> getPhase(ComplexArr arr);
std::vector<float> getPowerSpectrum(ComplexArr arr);

ComplexArr getZeros(size_t length);
ComplexArr getRange(float start, size_t length, float stride);
ComplexArr getWave(float freq, size_t size, float sampleRate);

std::vector<float> writeComplexToFloat(ComplexArr arr);
void printArr(ComplexArr arr);
void printArr(ComplexArr arr, int start, int ammount);
