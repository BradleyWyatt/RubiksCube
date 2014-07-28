#pragma once

#include <cmath>

#include <opencv2/core/core.hpp>
#include <opencv2/core/operations.hpp>

using namespace cv;
using namespace std;

class Circular {

private:

	Vec2d vectors[181];

	// Fill vectors with the 180 roots of unity
	void populate();

public:

	Circular();

	// Get the nth root of unity
	Vec2d getUnity(int n);

};