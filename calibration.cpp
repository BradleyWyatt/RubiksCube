#include "stdafx.h"

#include <fstream>
#include <iostream>
#include <cctype>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/operations.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>

#include "calibration.h"
#include "stringnames.h"

using namespace cv;
using namespace std;

Calibration::Calibration(VideoCapture &capture, Size &image_size) {
	this->capture = capture;
	this->image_size = image_size;

	board_size = Size(9,6);
	square_size = 1.f;

	int nframes = 10;
	int delay = 1000;
	clock_t prev_timestamp = 0;
	vector<vector<Point2f>> image_points;
}

bool Calibration::getCalibrationSettingsFromFile(Mat &intrinsic, Mat &dist_coeffs) {
	ifstream settings_file(calibration_file);
	bool success = false;
	if (settings_file.good()) {
		success = true;
		for (int i = 0; i < 5; i++) {
			settings_file >> dist_coeffs.ptr<double>(i)[0];
		}
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				settings_file >> intrinsic.ptr<double>(i)[j];
			}
		}
	}
	settings_file.close();
	return success;
}

void Calibration::findCalibrationValues(Mat &intrinsic, Mat &dist_coeffs, vector<vector<Point2f>> &image_points) {
	vector<vector<Point3f>> object_points(1);
	object_points[0].resize(0);

	for (int i = 0; i < board_size.height; i++) {
		for (int j = 0; j < board_size.width; j++) {
			object_points[0].push_back(Point3f(float(j*square_size), float(i*square_size), 0));
		}
	}

	object_points.resize(image_points.size(), object_points[0]);

	vector<Mat> rvecs, tvecs;
    intrinsic = Mat::eye(3, 3, CV_64F);
    dist_coeffs = Mat::zeros(8, 1, CV_64F);
	cv::calibrateCamera(object_points, image_points, image_size, intrinsic, dist_coeffs, rvecs, tvecs, CALIB_FIX_K4|CALIB_FIX_K5);

	ofstream outputFile;
	outputFile.open(calibration_file);
	for (int i = 0; i < dist_coeffs.rows; i++) {
		outputFile << dist_coeffs.ptr<double>(i)[0] << endl;
	}
	for (int i = 0; i < intrinsic.rows; i++) {
		for (int j = 0; j < intrinsic.cols; j++) {
			outputFile << intrinsic.ptr<double>(i)[j] << endl;
		}
	}
	outputFile.close();
}

void Calibration::calibrateCamera(Mat &intrinsic, Mat &dist_coeffs) {
	int delay = 1000;
	clock_t prev_timestamp = 0;
	vector<vector<Point2f>> image_points;

	Mat frame;
	Mat scaled;
	Mat grey_image;
	cout << chessboard_width << endl;
	int width = 0;
	while (!(cin >> width) || width <= 1) {
		cout << enter_integer << endl;
		cout << chessboard_width << endl;
		cin.clear();
		cin.ignore(1000, '\n');
	}
	int height = 0;
	cout << chessboard_height << endl;
	while (!(cin >> height) || height <= 1) {
		cout << enter_integer << endl;
		cout << chessboard_height << endl;
		cin.clear();
		cin.ignore(1000, '\n');
	}
	width--;
	height--;
	board_size = Size(width, height);
	int nframes = 840/(width*height);
	while (image_points.size() < nframes) {
		capture.read(frame);
		resize(frame, scaled, image_size);
		flip(scaled, scaled, 1);
		bool blink = false;

		vector<Point2f> points;
		cvtColor(scaled, grey_image, COLOR_BGR2GRAY);

		bool found = findChessboardCorners(scaled, board_size, points, CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_FAST_CHECK | CALIB_CB_NORMALIZE_IMAGE);

		if(found) {
			cornerSubPix(grey_image, points, Size(11,11), Size(-1,-1), TermCriteria(TermCriteria::EPS+TermCriteria::COUNT, 30, 0.1));
		}

		if(found && (!capture.isOpened() || clock() - prev_timestamp > delay*1e-3*CLOCKS_PER_SEC)) {
			image_points.push_back(points);
			prev_timestamp = clock();
			blink = true;
		}

		if (found) {
			drawChessboardCorners(scaled, board_size, Mat(points), found);
		}

		//string msg = "Camera Calibration";
		//int baseLine = 0;
		//Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
		//Point textOrigin(scaled.cols - 2*textSize.width - 10, scaled.rows - 2*baseLine - 10);

		//msg = format("%d/%d", (int)image_points.size(), nframes);

		//putText(scaled, msg, textOrigin, 1, 1, Scalar(0,255,0));

		if (blink) {
			bitwise_not(scaled, scaled);
		}
		waitKey(20);
		moveWindow(calibration_window, 800, 150);
		imshow(calibration_window, scaled);
	}
	findCalibrationValues(intrinsic, dist_coeffs, image_points);
	destroyWindow(calibration_window);
}