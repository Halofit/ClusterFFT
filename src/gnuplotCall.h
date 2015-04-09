#pragma once

#include "ComplexArr.h"

namespace plot {
	void testGnuplot();
	void drawHistogram(std::vector<float> arr);
	void drawHistogram(std::vector<float> arr, size_t start, size_t len);
	void drawLine(std::vector<float> arr);
	void drawLine(std::vector<float> arr, size_t start, size_t len);
}
