#include "stdafx.h"

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/core/operations.hpp>

#include "circular.h"

using namespace cv;
using namespace std;

Circular::Circular() {
	populate();
}

void Circular::populate() {
	double angle = 0;
	double increment = CV_PI/90;
	for (int i = 0; i < 180; i++) {
		vectors[i] = Vec2d(cos(angle), sin(angle));
		angle += increment;
	}
}

Vec2d Circular::getUnity(int n) {
	return vectors[n];
}