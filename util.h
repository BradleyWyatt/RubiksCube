#pragma once

#include <opencv2/core/core.hpp>

using namespace cv;

namespace util {

	// Returns the difference between prev and current, with thresholding
	Mat frameDifference(Mat prev, Mat current);

	// Filters current by using the images in prev
	Mat meanFilter(vector<Mat> prev, Mat current);

	// Length of line
	double length(const Vec4i& line);

	// Length of the line (x1,y1) (x2,y2)
	double length(int x1, int y1, int x2, int y2);

	// Returns the gradient of line
	double gradient(Vec4i* line);

	// Returns the distance the point at fraction s along the line (x1, y1) (x2, y2) is from the point (x3, y3)
	double distance(double s, int x1, int y1, int x2, int y2, int x3, int y3);

	// Finds the minimum distance between lines a and b. If trim is true, will trim the lines so that the endpoint of one is on the line of the other
	double minDistance(Vec4i& a, Vec4i& b, bool do_clip=false, int threshold=0);

	// Clips cd against ab. side = 0 if the lower x value point of cd is to be removed. side = 1 if the higher x value point of cd is to be removed.
	void clip(const Vec4i& ab, Vec4i& cd, int side);

	// Checks whether a and b share an endpoint
	bool endPoint(const Vec4i& a, const Vec4i& b);

	// Checks whether a and b intersect
	bool intersect(const Vec4i& a, const Vec4i& b);
	
	// Finds the median pixel based on hue within the mask
	Scalar median(Mat& image, Mat& mask);

}