#pragma once

#include "ComplexArr.h"

namespace plot {
	int testGnuplot();
	void drawHistogram(std::vector<float> arr);
	void drawHistogram(std::vector<float> arr, size_t start, size_t len);
}
