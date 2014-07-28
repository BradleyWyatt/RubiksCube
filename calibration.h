#pragma once

#include <cmath>
#include <time.h>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/core/operations.hpp>

using namespace cv;

class Calibration {

private:

	VideoCapture capture;
	Size image_size;
	Size board_size;
	float square_size;

	int nframes;
	int delay;
	clock_t prev_timestamp;

	// Calculates the calibration values and stores them in intrinisic and dist_coeffs
	void findCalibrationValues(Mat &intrinsic, Mat &dist_coeffs, vector<vector<Point2f>> &image_points);

public:

	Calibration(VideoCapture &capture, Size &image_size);

	// Runs the chessboard calibration and updates the intrinsic and undistortion matrices for use in undistorting the camera image
	void calibrateCamera(Mat &intrinsic, Mat &dist_coeffs);
	
	// Read calibration settings from a predetermined file and store them in intrinsic and dist_coeffs. Returns true if file exists
	bool getCalibrationSettingsFromFile(Mat &intrinsic, Mat &dist_coeffs);

};