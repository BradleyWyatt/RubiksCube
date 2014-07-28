#include "stdafx.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include <opencv2/core/core.hpp>
#include <opencv2/core/operations.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/video/tracking.hpp>

#include "calibration.h"
#include "colourgrouper.h"
#include "cube.h"
#include "cubedetector.h"
#include "cubedrawernet.h"
#include "cubesolver.h"
#include "position.h"
#include "util.h"

#include "evaluation.h"

#include "stringnames.h"

#define INF 999999

using namespace cv;
using namespace std;
using namespace util;

int _tmain(int argc, _TCHAR* argv[]) {
	
	//////////////////////////////////
	// Solver evaluation
	//evaluation::solver(10000000, true);
	//evaluation::solver(10000000, false);
	//////////////////////////////////
	
	//evaluation::generateFFmpeg();
	//evaluation::processData();

	cout << name << instructions << endl;
	CubeDetector cube_detector;
	cube_detector.startDetection();
}

CubeDetector::CubeDetector() {
	height = 200;
	//name = "Input image";
	input_cube = input_cube_string;
	detected_cube = detected_cube_string;
	cube_grid = cube_grid_string;
	prev_frames = 100;
	angle_threshold = 5;
	distance_threshold = 10;
	angle_count_size = 60;
	complete = false;
	saturation_value_difference = 50;

	grid_obtained = false;
	face_index_global = 0;
	
	skin_mean = Scalar(0,0,0);
	skin_diff = Scalar(0,0,0);
	skin_stdev = Scalar(0,0,0);
	total_time = 0;
	moves = 0;
	skin_detect_moves = 0;

	intrinsic = Mat::eye(3, 3, CV_64F);
	dist_coeffs = Mat::zeros(5, 1, CV_64F);

	ksize_slider = 1;
	gsize_slider = 1;
	sigma_slider = 0;
	rho_slider = 0;
	theta_slider = 0;
	val_slider = 8;
	threshold_slider = 47;
	length_slider = 20 + height/40; // Set to 25/30 if height = 200/400
	gap_slider = 4;
	morph_slider = 0;
	angle_slider = 9;
	numlines_slider = 2;
	low = 0;
	high = 0;
}

void CubeDetector::startDetection() {
	//	namedWindow(name, CV_WINDOW_AUTOSIZE);
	Mat frame;
	Mat scaled;
	string id ="";
	//cin >> id;
	VideoCapture capture;
	bool opened = false;
	for (int i = 0; !opened; i++) {
		cout << "Attempting to opening video device: " << i << endl;
		opened = capture.open(i);
		string e = opened ? "Success" : "Failed";
		cout << e << endl;
	}
	int i = 0;
	while (frame.empty()) {
		i++;
		if (i % 10000000 == 0) {
			i = 0;
			cout << "Frame empty" << endl;
		}
		capture >> frame;
	}
	double ratio = (double) frame.rows/height;
	width = (double)frame.cols/ratio;
	size = Size(width, height);
	CubeDrawerNet cube_drawer_net;
	Mat cube_image = cube_drawer_net.drawCubeOutline(height);
	Position position;
	position.getPositionFromFile();
	vector<vector<pair<int,int>>> window_positions = position.getPositions();
	imshow(input_cube, cube_image);
	imshow(detected_cube, cube_image);
	moveWindow(input_cube, window_positions[1][1].first, window_positions[1][1].second);
	moveWindow(detected_cube, window_positions[2][1].first, window_positions[2][1].second);
	resize(frame, scaled, size);
	flip(scaled, scaled, 1);
	imshow(cube_grid, scaled);
	moveWindow(cube_grid, window_positions[1][0].first, window_positions[1][0].second);
	bool detecting_cube = true;
	int face_index = 0;
	Calibration calibration(capture, size);
	calibrated = calibration.getCalibrationSettingsFromFile(intrinsic, dist_coeffs);
	Cube cube;
	thread t(&CubeDetector::captureCube, this, capture);
	
	// Get instruction images for initialisation
	Size instruction_size(200, 200);
	Mat initialisation_images[6];
	Mat instruction_image = imread("images\\blank.png");
	resize(instruction_image, instruction_image, instruction_size);
	initialisation_images[0] = instruction_image;
	instruction_image = imread("images\\r50.png");
	resize(instruction_image, instruction_image, instruction_size);
	for (int i = 1; i <= 3; i++) {
		initialisation_images[i] = instruction_image;
	}
	instruction_image = imread("images\\r00.png");
	resize(instruction_image, instruction_image, instruction_size);
	initialisation_images[4] = instruction_image;
	instruction_image = imread("images\\r01.png");
	resize(instruction_image, instruction_image, instruction_size);
	initialisation_images[5] = instruction_image;
	imshow(instruction, instruction_image);
	moveWindow(instruction, window_positions[0][1].first, window_positions[0][1].second);

	while (!complete) {
		while (detecting_cube) {
			if (face_index <= 5) {
				imshow(instruction, initialisation_images[face_index]);
			}
			int key = cv::waitKey(0);
			if (key == 99) {
				calibration.calibrateCamera(intrinsic, dist_coeffs);
				// Lock calibrated mutex
				calibrated_mutex.lock();
				calibrated = true;
				// Unlock
				calibrated_mutex.unlock();
				cout << calibrated_message << endl;
			} else if (key == 112) {
				position.changePositions();
				window_positions = position.getPositions();
				moveWindow(input_cube, window_positions[1][1].first, window_positions[1][1].second);
				moveWindow(detected_cube, window_positions[2][1].first, window_positions[2][1].second);
				moveWindow(cube_grid, window_positions[1][0].first, window_positions[1][0].second);
				moveWindow(instruction, window_positions[0][1].first, window_positions[0][1].second);
			}
			grid_obtained_mutex.lock();
			if (key == 32 && face_index == 6) {
				detecting_cube = false;
				face_index = 0;
			} else if (key == 32 && grid_obtained) {
				Mat hsv_mat(Size(9,1), CV_8UC3);
				Mat bgr_mat(Size(9,1), CV_8UC3);
				///	cout << face_index << endl;
				// Lock
				last_detected_mutex.lock();
				for (int i = 0; i < last_detected_grid.size(); i++) {
					vector<Point>& points = last_detected_grid[i];
					Mat sticker_mask = Mat::zeros(size, CV_8U);
					fillConvexPoly(sticker_mask, points, points.size(), 0);
					hsv_values[face_index][i] = median(last_detected_cube, sticker_mask);
					hsv_mat.at<Vec3b>(0,i) = Vec3b(hsv_values[face_index][i][0], hsv_values[face_index][i][1], hsv_values[face_index][i][2]);
				}
				// Unlock
				last_detected_mutex.unlock();
				cvtColor(hsv_mat, bgr_mat, CV_HSV2BGR);
				for (int i = 0; i < 9; i++) {
					///	cout << hsv_values[face_index][i] << endl;
					cube_drawer_net.colourSticker(face_index, i, Scalar(bgr_mat.at<Vec3b>(0,i)));
				}
				imshow(input_cube, cube_image);
				face_index++;
				grid_obtained = false;
			} else if (key == 8 && face_index > 0) {
				//cout << "Deleting previous image" << endl;
				face_index--;
				for (int i = 0; i < 9; i++) {
					bgr_values[face_index][i] = Scalar(255,255,255);
				}
				cube_image = cube_drawer_net.colourFace(face_index, bgr_values[face_index]);
				imshow(input_cube, cube_image);
				grid_obtained = false;
			}
			grid_obtained_mutex.unlock();
			face_index_mutex.lock();
			face_index_global = face_index;
			face_index_mutex.unlock();
		}
		int white_side = findWhiteSide(hsv_values);

		///////////////////////////////////
		// COLOUR GROUPER EVALUATION METHOD
		//evaluation::colours(hsv_values, id);
		///////////////////////////////////

		int positions[6][9];
		Scalar centre_colours[6];
		bool constraints_met = false;
		Scalar hsv_values_copy[6][9];

		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 9; j++) {
				hsv_values_copy[i][j] = hsv_values[i][j];
			}
		}
		int num_attempts = 0;
		Scalar rgb_values[6][9];
		Scalar lab_values[6][9];
		Scalar rgb_values_copy[6][9];
		Scalar lab_values_copy[6][9];
		int colour_space = 0;
		for (; colour_space < 4; colour_space++) {
			num_attempts = 0;
			if (colour_space == 2) {
				for (int i = 0; i < 6; i++) {
					for (int j = 0; j < 9; j++) {
						Mat m1(Size(1,1), CV_8UC3, hsv_values[i][j]);
						Mat m2;
						cvtColor(m1, m2, CV_HSV2BGR);
						Vec3b pixel = m2.at<Vec3b>(0,0);
						rgb_values[i][j] = Scalar(pixel[0], pixel[1], pixel[2]);
						rgb_values_copy[i][j] = rgb_values[i][j];
					}
				}
			} else if (colour_space == 3) {
				for (int i = 0; i < 6; i++) {
					for (int j = 0; j < 9; j++) {
						Mat m1(Size(1,1), CV_8UC3, rgb_values[i][j]);
						Mat m2;
						cvtColor(m1, m2, CV_BGR2Lab);
						Vec3b pixel = m2.at<Vec3b>(0,0);
						lab_values[i][j] = Scalar(pixel[0], pixel[1], pixel[2]);
						lab_values_copy[i][j] = lab_values[i][j];
					}
				}
			}
			while (!constraints_met && num_attempts < 10) {
				num_attempts++;
				ColourGrouper colour_grouper;
				if (colour_space == 0) {
					colour_grouper.groupColours(cube_image, hsv_values, positions, centre_colours, white_side);
					constraints_met = colour_grouper.fitColours(positions, hsv_values, centre_colours);
				} else if (colour_space == 1) {
					colour_grouper.groupColours(cube_image, hsv_values, positions, centre_colours, true);
					constraints_met = colour_grouper.fitColours(positions, hsv_values, centre_colours);
				} else if (colour_space == 2) {
					colour_grouper.groupColours(cube_image, rgb_values, positions, centre_colours, false);
					constraints_met = colour_grouper.fitColours(positions, rgb_values, centre_colours);
				} else {
					colour_grouper.groupColours(cube_image, lab_values, positions, centre_colours, false);
					constraints_met = colour_grouper.fitColours(positions, lab_values, centre_colours);
				}
				if (constraints_met) {
					cube = Cube(positions, centre_colours);
					CubeSolver cube_solver(&cube);
					if (cube_solver.solve()) {
						break;
					} else {
						constraints_met = false;
					}
				}
				if (colour_space <= 1) {
					for (int i = 0; i < 6; i++) {
						for (int j = 0; j < 9; j++) {
							hsv_values[i][j] = hsv_values_copy[i][j];
						}
					}
				} else {
					for (int i = 0; i < 6; i++) {
						for (int j = 0; j < 9; j++) {
							rgb_values[i][j] = rgb_values_copy[i][j];
							lab_values[i][j] = lab_values_copy[i][j];
						}
					}
				}
			}
			if (constraints_met) {
				break;
			}
		}
		int key;
		if (num_attempts >= 10) {
			cout << failed_initialisation_message << endl;
			key = 8;
		} else {
			if (colour_space == 1 || colour_space == 3) {
				for (int i = 0; i < 6; i++) {
					Mat m1(Size(1,1), CV_8UC3, centre_colours[i]);
					Mat m2;
					if (colour_space == 1) {
						cvtColor(m1, m2, CV_HSV2BGR);
					} else {
						cvtColor(m1, m2, CV_Lab2BGR);
					}
					Vec3b pixel = m2.at<Vec3b>(0,0);
					centre_colours[i] = Scalar(pixel[0], pixel[1], pixel[2]);
				}
			}
			imshow(detected_cube, cube_drawer_net.colourCube(positions, centre_colours));
	//		key = waitKey(0);
		}
	//	if (key == 32) {
			cube.setDetector(this);
			destroyWindow(detected_cube);
			destroyWindow(input_cube);
			cube.replayMoves(positions, centre_colours);
			CubeDrawerUser cube_drawer_user(200);
			cube.getPositions(positions);
			imshow(current_user_view, cube_drawer_user.colourCube(positions, centre_colours));
			
			moveWindow(current_user_view, window_positions[1][1].first, window_positions[1][1].second);
			CubeDrawerNet cube_drawer_net(200);
			imshow(cube_net, cube_drawer_net.colourCube(positions, centre_colours));
			moveWindow(cube_net, window_positions[2][0].first, window_positions[2][0].second);
			cout << space_to_retry_message << endl << endl;
			key = waitKey(0);
			if (key != 32) {
				complete = true;
				cout << closing_message << endl;
			}
	//	}
		// Either cube is completed, or the user is not happy with initialisation. Reset
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 9; j++) {
				bgr_values[i][j] = Scalar(255,255,255);
			}
		}
		for (int i = 0; i < 6; i++) {
			cube_image = cube_drawer_net.colourFace(i, bgr_values[i]);
		}
		imshow(input_cube, cube_image);
		detecting_cube = true;
	}
	t.join();
}

void CubeDetector::captureCube(VideoCapture& capture) {
	Mat frame;
	Mat scaled;
	Mat grey_image;
	Mat out;
	Mat total;
	int count = 0;
	vector<Mat> prev_colour;
	Mat frame32f;
	capture.read(frame);
	resize(frame, scaled, size);
	scaled.convertTo(frame32f, CV_32F);
	cvtColor(scaled, grey_image, CV_BGR2GRAY);
	vector<vector<Point>> current_grid;
	Mat prev_grey;
	vector<Point2f> lk_points;
	vector<Point2f> prev_lk_points;
	
	bool detected = false;
	bool first_time = true;
	Mat background = scaled.clone();
	cvtColor(background, background, CV_RGB2GRAY);
	double alpha = 0.05;
	clock_t start = clock();
//	int a = 0;
//	int c = 0;
//	int tt = 0;
	while (capture.read(frame) && !complete) {
		resize(frame, scaled, size);
		calibrated_mutex.lock();
		if (calibrated) {
			Mat distorted = scaled.clone();
			undistort(distorted, scaled, intrinsic, dist_coeffs);
		}
		calibrated_mutex.unlock();
		flip(scaled, scaled, 1);
		/*int percent = 90;
		int delta = height*percent/200;
		imshow("Press here", scaled);
		clock_t now = clock();
		if (int (now-start)/CLOCKS_PER_SEC >= 2*tt) {
			cout << "Next " << tt << endl;
			tt++;
		}
		if (float (now-start)/CLOCKS_PER_SEC > 100) {
			cout << c << endl;
		} else {
			int b = waitKey(1);
			if (b != -1) {
				cout << "Captured" << endl;
				c++;
			}
		}
		

		rectangle(scaled, Point(width/2 - delta, height/2 - delta), Point(width/2 + delta, height/2 + delta), Scalar(0,0,255), 3);*/
		
	//	imshow("input", scaled);
		cvtColor(scaled, grey_image, CV_BGR2GRAY);
		background = background*(1-alpha) + grey_image*alpha;
	//	Mat grey_total;
	//	total.convertTo(grey_total, CV_8U);
	//	cvtColor(grey_total, grey_total, CV_BGR2GRAY);
		
	//	Mat mask = frameDifference(grey_total, grey_image);
		Mat mask = frameDifference(background, grey_image);
	//	imshow("background mask undilated", mask);
		dilate(mask, mask, Mat());
	//	imshow("background mask", mask);
		//imshow(name, scaled);
		Mat temp;
		scaled.convertTo(temp, CV_32F);

		cvtColor(scaled, out, CV_BGR2GRAY);
		GaussianBlur(out, out, Size(2*gsize_slider+3, 2*gsize_slider+3), sigma_slider+1);
	//	Mat out_canny;
	//	Canny(out, out_canny, 50, 150);
		Laplacian(out, out, 5, 2*ksize_slider+1);
	//	waitKey(25);
		compare(out, Scalar::all(val_slider), out, CV_CMP_GT);
	//	imshow("Pre-background removal laplacian", out);
	//	imshow("Pre-background removal canny", out_canny);
		threshold(out, out, 2,255,THRESH_BINARY);
		
		bitwise_and(out, mask, out);
	//	bitwise_and(out_canny, mask, out_canny);
		out.copyTo(out, mask);
	//	out_canny.copyTo(out_canny, mask);
		waitKey(1);
	//	imshow("Laplacian", out);
	//	imshow("Canny", out_canny);
		//imshow("Test",out);
		Mat grid_image = scaled.clone();
		if (detected) {
			vector<uchar> status;
			vector<float> err;
			if (prev_grey.empty()) {
				grey_image.copyTo(prev_grey);
			}
			if (prev_lk_points.empty()) {
				last_detected_mutex.lock();
				prev_lk_points.push_back(Point((last_detected_grid[0][0].x+last_detected_grid[4][2].x)/2,(last_detected_grid[0][0].y+last_detected_grid[4][2].y)/2));
				prev_lk_points.push_back(Point((last_detected_grid[2][1].x+last_detected_grid[4][3].x)/2,(last_detected_grid[2][1].y+last_detected_grid[4][3].y)/2));
				prev_lk_points.push_back(Point((last_detected_grid[6][3].x+last_detected_grid[4][1].x)/2,(last_detected_grid[6][3].y+last_detected_grid[4][1].y)/2));
				prev_lk_points.push_back(Point((last_detected_grid[8][2].x+last_detected_grid[4][0].x)/2,(last_detected_grid[8][2].y+last_detected_grid[4][0].y)/2));
				last_detected_mutex.unlock();
			}
			calcOpticalFlowPyrLK(prev_grey, grey_image, prev_lk_points, lk_points, status, err);
			for (Point2f point : lk_points) {
				if (point.x < 0 || point.y < 0 || point.x > size.width || point.y > size.height) {
					detected = false;
				}
			}
			double min_distance = INF;
			double max_distance = 0;
			for (int i = 0; i < 4; i++) {
				for (int j = i+1; j < 4; j++) {
					double distance = length(lk_points[i].x, lk_points[i].y, lk_points[j].x, lk_points[j].y);
					max_distance = max(max_distance, distance);
					min_distance = min(min_distance, distance);
				}
			}
			if (max_distance > 2*min_distance) {
				detected = false;
			}
			// Calculate angle between diagonals
			Vec2i vec_A(lk_points[3].x - lk_points[0].x, lk_points[3].y - lk_points[0].y);
			Vec2i vec_B(lk_points[2].x - lk_points[1].x, lk_points[2].y - lk_points[1].y);
			double theta = (vec_A[0]*vec_B[0] + vec_A[1]*vec_B[1])/(length(lk_points[0].x, lk_points[0].y, lk_points[3].x, lk_points[3].y)*length(lk_points[1].x, lk_points[1].y, lk_points[2].x, lk_points[2].y));
			//cout << vec_A[0] << " " << vec_A[1] << " " << vec_B[0] << " " << vec_B[1] << " " << acos(theta)*180/CV_PI << endl;
			if (abs(theta) > 0.09) { // 85 degrees
				/*ofstream myfile;
				myfile.open("longtime_detection.txt", std::ofstream::out | std::ofstream::app);
				clock_t t = clock()-start;
				myfile << ((float)t)/CLOCKS_PER_SEC << endl;
				myfile.close();*/
				detected = false;
			}
			else if (!detected || !intersect(Vec4i(lk_points[0].x, lk_points[0].y, lk_points[3].x, lk_points[3].y), Vec4i(lk_points[1].x, lk_points[1].y, lk_points[2].x, lk_points[2].y))) {
				prev_lk_points.clear();
				lk_points.clear();
				prev_grey.release();
				/*ofstream myfile;
				myfile.open("longtime_detection.txt", std::ofstream::out | std::ofstream::app);
				clock_t t = clock()-start;
				myfile << ((float)t)/CLOCKS_PER_SEC << endl;
				myfile.close();*/
				detected = false;
			} else {
				vector<vector<Point>> current_grid;
				//	for (Point2f p : lk_points) {
				//		 circle(grid_image, p, 3, Scalar(0,255,0), -1, 8);
				//	}
				Vec4i line_A(2*lk_points[0].x - lk_points[2].x, 2*lk_points[0].y - lk_points[2].y, 2*lk_points[2].x - lk_points[0].x, 2*lk_points[2].y - lk_points[0].y);
				Vec4i line_B(2*lk_points[0].x - lk_points[1].x, 2*lk_points[0].y - lk_points[1].y, 2*lk_points[1].x - lk_points[0].x, 2*lk_points[1].y - lk_points[0].y);
				drawGridLines(line_A, line_B, 1, grid_image);
				drawGridLines(line_B, line_A, 1, grid_image);
				//	line(grid_image, Point(line_A[0], line_A[1]), Point(line_A[2], line_A[3]), Scalar(0,255,0));
				//	line(grid_image, Point(line_B[0], line_B[1]), Point(line_B[2], line_B[3]), Scalar(0,255,0));
				double intersect_x = 1;
				double intersect_y = 1;
				double start_x = lk_points[0].x;
				double start_y = lk_points[0].y;
				addToGrid(line_A, line_B, intersect_x, intersect_y, start_x, start_y, current_grid);
				//	for (vector<Point> p : last_detected_grid) {
				//		line(grid_image, p[0], p[1], Scalar(0,255,0));
				//		line(grid_image, p[1], p[2], Scalar(0,255,0));
				//		line(grid_image, p[2], p[3], Scalar(0,255,0));
				//		line(grid_image, p[3], p[0], Scalar(0,255,0));
				//	}
				last_detected_mutex.lock();
				face_index_mutex.lock();
				last_detected_cube = scaled.clone();
				cvtColor(last_detected_cube, last_detected_cube, CV_BGR2HSV);
				last_detected_grid = orientateGrid(current_grid, face_index_global);
				face_index_mutex.unlock();
				last_detected_mutex.unlock();

				grid_obtained_mutex.lock();
				grid_obtained = true;
				grid_obtained_mutex.unlock();

				imshow(cube_grid, grid_image);
				std::swap(lk_points, prev_lk_points);
				prev_grey = grey_image.clone();
			}
		}
	/*	Mat h11 = imread("image1.png", CV_LOAD_IMAGE_GRAYSCALE);
		Mat h22 = imread("image2.png", CV_LOAD_IMAGE_GRAYSCALE);
		Mat h1 = h11.clone();
		Mat h2 = h22.clone();*/
		vector<Vec4i> lines;
		HoughLinesP(out, lines, rho_slider+1, CV_PI/45, 101-threshold_slider, length_slider+1, gap_slider+1);
	//	Mat h1 = scaled.clone();
	//	for (Vec4i line : lines) {
	//		cv::line(h1, Point(line[0], line[1]), Point(line[2], line[3]), Scalar(0,0,255));
	//	}
	//	imshow("Hough Laplacian", h1);
	/*	HoughLinesP(h2, lines, rho_slider+1, CV_PI/45, 101-threshold_slider, length_slider+1, gap_slider+1);
		h2 = imread("image2.png", CV_LOAD_IMAGE_COLOR);
		for (Vec4i line : lines) {
			cv::line(h2, Point(line[0], line[1]), Point(line[2], line[3]), Scalar(0,0,255));
		}
		imshow("Hough Canny", h2);*/
		vector<pair<Vec4i, double>> angles;
		for (const Vec4i& l : lines) {
			double gradient = 0;
			// Check for infinite gradient
			if (l[2] - l[0] == 0) {
				gradient = INF;
			} else {
				gradient = (double)(l[3]-l[1])/(l[2]-l[0]);
			}
			angles.push_back(make_pair(l, atan(gradient)));
		}
		sort(angles.begin(), angles.end(), &CubeDetector::pointComparator);

		removeSmallAngles(angles);

		vector<int> angle_count;
		for (int i = 0; i < angle_count_size; i++) {
			angle_count.push_back(0);
		}
		for (const pair<Vec4i, double>& p : angles) {
			double angle = p.second;
			angle_count[(int)((angle+CV_PI/2)*angle_count.size()/CV_PI)]++;
		}

		double low_angle;
		double high_angle;
		int num_lines = findCubeAngles(angle_count, low_angle, high_angle);

		vector<pair<Vec4i, bool>> lines_A;
		vector<pair<Vec4i, bool>> lines_B;
		int s = 0;
		bool grid_this_cycle = false;
		if (!detected && num_lines > 0) {
			filterLines(angles, low_angle, high_angle, lines_A, lines_B);
			joinPoints(lines_A, lines_B);
			s = lines_A.size() + lines_B.size();
			vector<tuple<Vec4i, Vec4i, double>> all_pairs = getAllPairs(lines_A, lines_B);
			sort(all_pairs.begin(), all_pairs.end(), &CubeDetector::pairsLengthComparator);
			Mat grid_image_temp = scaled.clone();
			current_grid = findGrid(all_pairs, lines_A, lines_B, grid_image_temp);
			if (!current_grid.empty()) {
				grid_this_cycle = true;
				grid_obtained_mutex.lock();
				grid_obtained = true;
				grid_obtained_mutex.unlock();
				// Lock
				last_detected_mutex.lock();
				face_index_mutex.lock();
				last_detected_cube = scaled.clone();
				cvtColor(last_detected_cube, last_detected_cube, CV_BGR2HSV);
				last_detected_grid = orientateGrid(current_grid, face_index_global);
				// Unlock
				face_index_mutex.unlock();
				last_detected_mutex.unlock();
				grid_image = grid_image_temp.clone();
				if (first_time) {
					first_time = false;
				/*	ofstream myfile;
					myfile.open("detection_dist_" + to_string(percent) + ".txt", std::ofstream::out | std::ofstream::app);
					clock_t t = clock()-start;
					myfile << ((float)t)/CLOCKS_PER_SEC << endl;
					myfile.close();*/
				}
				detected = true;
				prev_lk_points.clear();
				prev_grey = grey_image.clone();
			}
		}
		if (s > 20) {
			if (val_slider < 255) {
				val_slider++;
			}
		} else if (grid_this_cycle || s < 10) {
			if (val_slider > 0) {
				val_slider--;
			}

		}
		imshow(cube_grid, grid_image);
	}
}

void CubeDetector::checkColours(int positions[6][9], Scalar colours[6], vector<int> updated) {
	bool match = false;
	Mat hsv_mat(Size(6,1), CV_8UC3);
	Mat bgr_mat(Size(6,1), CV_8UC3);
	for (int i = 0; i < 6; i++) {
		bgr_mat.at<Vec3b>(0,i) = Vec3b(colours[i][0], colours[i][1], colours[i][2]);
	}
	cvtColor(bgr_mat, hsv_mat, CV_BGR2HSV);
	bool first_detect = true;
	Mat last_detected;
	while (!match) {
		match = true;
		bool next_grid_obtained = false;
		while (!next_grid_obtained) {
			grid_obtained_mutex.lock();
			next_grid_obtained = grid_obtained;
			grid_obtained_mutex.unlock();
			total_time++;
			waitKey(25);
		}
		grid_obtained_mutex.lock();
		grid_obtained = false;
		grid_obtained_mutex.unlock();
		last_detected_mutex.lock();
		last_detected = last_detected_cube.clone();
		last_detected_mutex.unlock();
		if (updated.size() == 0) {
			waitKey(25*total_time/moves);
			total_time += ((int) total_time)/moves;
		} else {
			Mat complete_mask = Mat::zeros(size, CV_8U);
			for (int i : updated) {
				vector<Point>& points = last_detected_grid[i];
				Mat sticker_mask = Mat::zeros(size, CV_8U);
				if (sticker_mask.rows > 0 && sticker_mask.cols > 0 && points.size() > 0) {
					for (Point p : points) {
						if (p.x < 0 || p.y < 0) {
							match = false;
							break;
						}
					}
					Point arr_points[1][4];
					arr_points[0][0] = points[0];
					arr_points[0][1] = points[1];
					arr_points[0][2] = points[2];
					arr_points[0][3] = points[3];
					const Point* ppt[1] = { arr_points[0] };
					int npt[] = { 4 };
					fillPoly(sticker_mask, ppt, npt, 1, Scalar(255,255,255));
				//	fillConvexPoly(sticker_mask, points, Scalar(255));
					Scalar range = skin_stdev;
					range[0] *= 1.5;
					range[1] *= 1.5;
					range[2] *= 1.5;
					Scalar upper_bound(skin_mean[0]+range[0], skin_mean[1]+range[1], skin_mean[2]+range[2]);
					Scalar lower_bound(skin_mean[0]-range[0], skin_mean[1]-range[1], skin_mean[2]-range[2]);
					//	cout << upper_bound[0] << " " << upper_bound[1] << " " << upper_bound[2] << endl;
					//	cout << lower_bound[0] << " " << lower_bound[1] << " " << lower_bound[2] << endl;
					Scalar hsv_expected = hsv_mat.at<Vec3b>(0, positions[0][i]);
					Scalar expected_upper_bound(hsv_expected[0]+range[0], hsv_expected[1]+range[1], hsv_expected[2]+range[2]);
					Scalar expected_lower_bound(hsv_expected[0]-range[0], hsv_expected[1]-range[1], hsv_expected[2]-range[2]);
					Mat skin_mask;
					// Mask of skin location
					cv::inRange(last_detected, lower_bound, upper_bound, skin_mask);
					Mat colour_mask;
					// Mask of expected colour
					cv::inRange(last_detected, expected_lower_bound, expected_upper_bound, colour_mask);
					bitwise_not(skin_mask, skin_mask);
					Mat toShowA = last_detected.clone();
					Mat toShow;
					cvtColor(toShowA, toShowA, CV_HSV2BGR);
					Mat skin_and_colour_mask;
					bitwise_or(skin_mask, colour_mask, skin_and_colour_mask);
					toShowA.copyTo(toShow, skin_mask);
					//bitwise_or(complete_mask, skin_and_colour_mask, complete_mask);
				//	imshow("Skin mask", toShow);
					bitwise_and(sticker_mask, skin_and_colour_mask, sticker_mask);
					bitwise_or(complete_mask, sticker_mask, complete_mask);
					Scalar hsv_actual = median(last_detected, sticker_mask);
					//Scalar hsv_expected = hsv_mat.at<Vec3b>(0, positions[0][i]);
					int hsv_actual_diff = hsv_actual[2]-hsv_actual[1];
					int hsv_expected_diff = hsv_expected[2] - hsv_expected[1];
					if ((min(abs(hsv_actual[0]-hsv_expected[0]),abs(hsv_actual[0]-hsv_expected[0]-180)) > 10) || abs(hsv_actual_diff-hsv_expected_diff) > 80) {
						//	cout << i << " " << hsv_expected << " " << hsv_expected_diff << " " << hsv_actual << "  " << hsv_actual_diff << endl;
						match = false;
						break;
					}
				} else {
					match = false;
					break;
				}
			}
			//Mat toShowB = last_detected.clone();
			//cvtColor(toShowB, toShowB, CV_HSV2BGR);
			//Mat toShowC;
			//toShowB.copyTo(toShowC, complete_mask);
			//imshow("Skin mask1", toShowC);
		}
		if (match && first_detect) {
			match = false;
			updated.clear();
			for (int i = 0; i < 9; i++) {
				updated.push_back(i);
			}
		}
		first_detect = false;
	}
	moves++;
	Scalar total(0,0,0);
	for (int i = 0; i < 9; i++) {
		bool check_passed = true;
		//	for (int update : updated) {
		//		if (i == update) {
		//			check_passed = false;
		//			break;
		//		}
		//	}
		vector<Point>& points = last_detected_grid[i];
		Mat sticker_mask = Mat::zeros(size, CV_8U);
		if (sticker_mask.rows > 0 && sticker_mask.cols > 0 && points.size() > 0) {
			for (Point p : points) {
				if (p.x < 0 || p.y < 0) {
					check_passed = false;
					break;
				}
			}
			if (check_passed) {
				Point arr_points[1][4];
				arr_points[0][0] = points[0];
				arr_points[0][1] = points[1];
				arr_points[0][2] = points[2];
				arr_points[0][3] = points[3];
				const Point* ppt[1] = { arr_points[0] };
				int npt[] = { 4 };
				fillPoly(sticker_mask, ppt, npt, 1, Scalar(255,255,255));
		//		fillConvexPoly(sticker_mask, points, Scalar(255));
				Scalar hsv_expected = hsv_mat.at<Vec3b>(0, positions[0][i]);
				Scalar upper_bound(hsv_expected[0]+10, hsv_expected[1]+40, hsv_expected[2]+40);
				Scalar lower_bound(hsv_expected[0]-10, hsv_expected[1]-40, hsv_expected[2]-40);
				Mat skin_mask;
				cv::inRange(last_detected, lower_bound, upper_bound, skin_mask);
				if (hsv_expected[0]-10 < 0 || hsv_expected[0]+10 > 180) {
					if (hsv_expected[0]-10 < 0) {
						upper_bound[0] += 180;
						lower_bound[0] += 180;
					} else if (hsv_expected[0]+10 > 180) {
						upper_bound[0] -= 180;
						lower_bound[0] -= 180;
					}
					Mat skin_mask_temp;
					cv::inRange(last_detected, lower_bound, upper_bound, skin_mask_temp);
					bitwise_or(skin_mask, skin_mask_temp, skin_mask);
				}
				bitwise_not(skin_mask, skin_mask);
				bitwise_and(sticker_mask, skin_mask, skin_mask);
				MatIterator_<uchar> s_it = sticker_mask.begin<uchar>();
				MatIterator_<uchar> mask_it = skin_mask.begin<uchar>();
				MatIterator_<uchar> end = skin_mask.end<uchar>();
				int non_zero = 0;
				int sticker_size = 0;
				for (; mask_it != end; mask_it++, s_it++) {
					if (*mask_it > 0) {
						non_zero++;
					}
					if (*s_it > 0) {
						sticker_size++;
					}
				}
				if (((double)non_zero)/sticker_size > 0.5) {
					MatIterator_<Vec3b> image_it = last_detected.begin<Vec3b>();
					mask_it = skin_mask.begin<uchar>();
					for (; mask_it != end; mask_it++, image_it++) {
						if (*mask_it > 0) {
							skin_detect_moves++;
							Scalar image_val(*image_it);
							Scalar last_skin_mean = skin_mean;
							skin_mean[0] += ((double)(image_val[0] - skin_mean[0]))/skin_detect_moves;
							skin_mean[1] += ((double)(image_val[1] - skin_mean[1]))/skin_detect_moves;
							skin_mean[2] += ((double)(image_val[2] - skin_mean[2]))/skin_detect_moves;
							skin_diff[0] += (image_val[0] - last_skin_mean[0])*(image_val[0] - skin_mean[0]);
							skin_diff[1] += (image_val[1] - last_skin_mean[1])*(image_val[1] - skin_mean[1]);
							skin_diff[2] += (image_val[2] - last_skin_mean[2])*(image_val[2] - skin_mean[2]);
						}
					}
				}
			}
		}
	}
	skin_stdev[0] = sqrt(skin_diff[0]/(skin_detect_moves-1));
	skin_stdev[1] = sqrt(skin_diff[1]/(skin_detect_moves-1));
	skin_stdev[2] = sqrt(skin_diff[2]/(skin_detect_moves-1));
	/*ofstream myfile;
	myfile.open ("participant.txt", std::ofstream::out | std::ofstream::app);
	myfile << total_time*25 << endl;
	myfile.close();*/
	// cout << skin_mean << endl;
	// cout << skin_diff << endl;
	// cout << skin_stdev << endl << endl;
}

vector<vector<Point>> CubeDetector::orientateGrid(vector<vector<Point>> grid, int index) {
	int lowest_x_index = findLowestXindex(grid);
	int lowest_y_index = findLowestYindex(grid);
	double m = ((double) grid[0][0].y-grid[0][1].y)/(grid[0][0].x-grid[0][1].x);
	bool rotate = (0 <= m && m <= 1) || (m <= -1);
	if (grid[0][0].x == grid[0][1].x) {
		rotate = true;
	}
	if (lowest_x_index == lowest_y_index) {
		if (lowest_x_index == 0) {
			if (grid[2][0].x < grid[6][0].x) {
				flipGridVert(grid);
				rotateGrid(grid, 1);
			}
		} else if (lowest_x_index == 2) {
			if (grid[0][0].x < grid[8][0].x) {
				rotateGrid(grid, 1);
			} else {
				flipGridVert(grid);
			}
		} else if (lowest_x_index == 6) {
			if (grid[0][0].x < grid[8][0].x) {
				flipGridHor(grid);
			} else {
				rotateGrid(grid, 3);
			}
		} else if (lowest_x_index == 8) {
			if (grid[2][0].x < grid[6][0].x) {
				rotateGrid(grid, 2);
			} else {
				flipGridHor(grid);
				rotateGrid(grid, 2);
			}
		}
	} else {
		if (lowest_x_index == 0 && lowest_y_index == 6) {
			flipGridVert(grid);
			rotateGrid(grid, 1);
		} else if (lowest_x_index == 2 && lowest_y_index == 0) {
			flipGridVert(grid);
		} else if (lowest_x_index == 2 && lowest_y_index == 8) {
			rotateGrid(grid, 1);
		} else if (lowest_x_index == 6 && lowest_y_index == 0) {
			rotateGrid(grid, 3);
		} else if (lowest_x_index == 6 && lowest_y_index == 8) {
			flipGridHor(grid);
		} else if (lowest_x_index == 8 && lowest_y_index == 2) {
			flipGridHor(grid);
			rotateGrid(grid, 1);
		} else if (lowest_x_index == 8 && lowest_y_index == 6) {
			rotateGrid(grid, 2);
		}
		if (rotate) {
			rotateGrid(grid, 1);
		}
	//	if (index == 5) {
	//		rotateGrid(grid, 2);
	//	}
	}
	return grid;
}

void CubeDetector::rotateGrid(vector<vector<Point>>& grid, int rotations) {
	while (rotations > 0) {
		vector<Point> temp = grid[0];
		grid[0] = grid[2];
		grid[2] = grid[8];
		grid[8] = grid[6];
		grid[6] = temp;
		temp = grid[1];
		grid[1] = grid[5];
		grid[5] = grid[7];
		grid[7] = grid[3];
		grid[3] = temp;
		rotations--;
	}
}

void CubeDetector::flipGridHor(vector<vector<Point>>& grid) {
	vector<Point> temp = grid[0];
	grid[0] = grid[6];
	grid[6] = temp;
	temp = grid[1];
	grid[1] = grid[7];
	grid[7] = temp;
	temp = grid[2];
	grid[2] = grid[8];
	grid[8] = temp;
}

void CubeDetector::flipGridVert(vector<vector<Point>>& grid) {
	vector<Point> temp = grid[0];
	grid[0] = grid[2];
	grid[2] = temp;
	temp = grid[3];
	grid[3] = grid[5];
	grid[5] = temp;
	temp = grid[6];
	grid[6] = grid[8];
	grid[8] = temp;
}

int CubeDetector::findLowestXindex(const vector<vector<Point>>& grid) {
	int lowest_x = INF;
	int lowest_index = -1;
	for (int i = 0; i < grid.size(); i++) {
		const vector<Point>& points = grid[i];
		if (points[0].x < lowest_x) {
			lowest_x = points[0].x;
			lowest_index = i;
		}
	}
	return lowest_index;
}

int CubeDetector::findLowestYindex(const vector<vector<Point>>& grid) {
	int lowest_y = INF;
	int lowest_index = -1;
	for (int i = 0; i < grid.size(); i++) {
		const vector<Point>& points = grid[i];
		if (points[0].y < lowest_y) {
			lowest_y = points[0].y;
			lowest_index = i;
		}
	}
	return lowest_index;
}

int CubeDetector::findMaxSVdiff(Scalar values[][9], double& max_difference) {
	int max_index = -1;
	for (int i = 0; i < 6; i++) {
		if (max_difference < values[i][4][2] - values[i][4][1]) {
			max_index = i;
			max_difference = values[i][4][2] - values[i][4][1];
		}
	}
	return max_index;
}

int CubeDetector::findWhiteSide(Scalar values[][9]) {
	double saturation = saturation_value_difference;
	return findMaxSVdiff(values, saturation);
}

vector<vector<Point>> CubeDetector::findGrid(vector<tuple<Vec4i, Vec4i, double>>& all_pairs, vector<pair<Vec4i, bool>>& lines_A, vector<pair<Vec4i, bool>>& lines_B, Mat& mat) {
	vector<vector<Point>> grid;
	int max_score = -INF;
	int max_index = -1;
	for (int i = 0; i < all_pairs.size(); i++) {
		Vec4i& a = get<0>(all_pairs[i]);
		Vec4i& b = get<1>(all_pairs[i]);
		double gradient_A = ((double)(a[3]-a[1]))/(a[2]-a[0]);
		double intersect_A = a[1] - gradient_A*a[0];
		double gradient_B = ((double)(b[3]-b[1]))/(b[2]-b[0]);
		double intersect_B = b[1] - gradient_B*b[0];
		bool horizontal_A = (abs(gradient_A) > 1) ? true : false;
		bool horizontal_B = (abs(gradient_B) > 1) ? true : false;
		vector<double> b_intersect;
		for (pair<Vec4i, bool>& pair_A : lines_A) {
			Vec4i& current = pair_A.first;
			double gradient_A = ((double)(current[3]-current[1]))/(current[2]-current[0]);
			double intersect_A = current[1] - gradient_A*current[0];
			double x = (intersect_B - intersect_A)/(gradient_A - gradient_B);
			double y = gradient_B*x + intersect_B;
			if (horizontal_B) {
				b_intersect.push_back(3*(x-b[0])/(b[2]-b[0]));
			} else {
				b_intersect.push_back(3*(y-b[1])/(b[3]-b[1]));
			}
		}
		vector<double> a_intersect;
		for (pair<Vec4i, bool>& pair_B : lines_B) {
			Vec4i& current = pair_B.first;
			double gradient_B = ((double)(current[3]-current[1]))/(current[2]-current[0]);
			double intersect_B = current[1] - gradient_B*current[0];
			double x = (intersect_A - intersect_B)/(gradient_B - gradient_A);
			double y = gradient_A*x + intersect_A;
			if (horizontal_A) {
				a_intersect.push_back(3*(x-a[0])/(a[2]-a[0]));
			} else {
				a_intersect.push_back(3*(y-a[1])/(a[3]-a[1]));
			}
		}
		int count = 0;
		for (double& value : a_intersect) {
			if (inRange(value)) {
				count++;
			} else {
				count--;
			}
		}
		for (double& value : b_intersect) {
			if (inRange(value)) {
				count++;
			} else {
				count--;
			}
		}
		max_score = max(max_score, count);
		if (count == max_score) {
			max_index = i;
		}
	}
	if (max_score > 0) {
		Vec4i& line_A = get<0>(all_pairs[max_index]);
		Vec4i& line_B = get<1>(all_pairs[max_index]);
		// line_A = Vec4i(100,50,101,200);
		// line_B = Vec4i(50,100,200,101);
		// TODO: make vertical and horizontal lines work
		//	Mat aaa = mat.clone();
		//	line(aaa, Point(line_A[0], line_A[1]), Point(line_A[2], line_A[3]), Scalar(0, 255, 0));
		//	line(aaa, Point(line_B[0], line_B[1]), Point(line_B[2], line_B[3]), Scalar(0, 255, 0));
		//	imshow("Cube axes", aaa);
		double gradient_A = ((double)(line_A[3]-line_A[1]))/(line_A[2]-line_A[0]);
		double intersect_A = line_A[1] - gradient_A*line_A[0];
		double gradient_B = ((double)(line_B[3]-line_B[1]))/(line_B[2]-line_B[0]);
		double intersect_B = line_B[1] - gradient_B*line_B[0];
		double x = (intersect_B - intersect_A)/(gradient_A - gradient_B);
		double y = gradient_B*x + intersect_B;
		bool horizontal_A = (abs(gradient_A) > 1) ? true : false;
		bool horizontal_B = (abs(gradient_B) > 1) ? true : false;
		double intersect_double_A = horizontal_B ? 3*(x-line_B[0])/(line_B[2]-line_B[0]) : 3*(y-line_B[1])/(line_B[3]-line_B[1]);
		double intersect_double_B = horizontal_A ? 3*(x-line_A[0])/(line_A[2]-line_A[0]) : 3*(y-line_A[1])/(line_A[3]-line_A[1]);
		drawGridLines(line_A, line_B, intersect_double_A, mat);
		addToGrid(line_A, line_B, intersect_double_A, intersect_double_B, x, y, grid);

		drawGridLines(line_B, line_A, intersect_double_B, mat);
	}
	return grid;
}

void CubeDetector::addToGrid(const Vec4i& line_A, const Vec4i& line_B, double& intersect_double_A, double& intersect_double_B, double& x, double& y, vector<vector<Point>>& grid) {
	if (inRange(intersect_double_A) && inRange(intersect_double_B)) {
		int intersect_A = intersect_double_A + 0.5;
		int intersect_B = intersect_double_B + 0.5;
		int dx_A = (line_A[2] - line_A[0])/3;
		int dy_A = (line_A[3] - line_A[1])/3;
		int dx_B = (line_B[2] - line_B[0])/3;
		int dy_B = (line_B[3] - line_B[1])/3;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				vector<Point> points;
				double index_Al = i - intersect_B + 0.25;
				double index_Ah = i - intersect_B + 0.75;
				double index_Bl = j - intersect_A + 0.25;
				double index_Bh = j - intersect_A + 0.75;
				points.push_back(Point(x + dx_A*index_Al + dx_B*index_Bl, y + dy_A*index_Al + dy_B*index_Bl));
				points.push_back(Point(x + dx_A*index_Al + dx_B*index_Bh, y + dy_A*index_Al + dy_B*index_Bh));
				points.push_back(Point(x + dx_A*index_Ah + dx_B*index_Bh, y + dy_A*index_Ah + dy_B*index_Bh));
				points.push_back(Point(x + dx_A*index_Ah + dx_B*index_Bl, y + dy_A*index_Ah + dy_B*index_Bl));
				grid.push_back(points);
			}
		}
	}
}

void CubeDetector::drawGridLines(const Vec4i& line_A, const Vec4i& line_B, double intersect_double, Mat& mat) {
	if (inRange(intersect_double)) {
		int intersect = intersect_double + 0.5;
		int dx = (line_B[2] - line_B[0])/3;
		int dy = (line_B[3] - line_B[1])/3;
		for (int i = 0; i <= 3; i++) {
			int index = i - intersect;
			line(mat, Point(line_A[0] + dx*index, line_A[1] + dy*index), Point(line_A[2] + dx*index, line_A[3] + dy*index), Scalar(0,255,0));
		}
	}
}

bool CubeDetector::inRange(double value) {
	double d = 0.2;
	double low0 = 0;
	double high0 = 0 + d;
	double low1 = 1 - d;
	double high1 = 1 + d;
	double low2 = 2 - d;
	double high2 = 2 + d;
	double low3 = 3 - d;
	double high3 = 3;
	return ((value >= low0 && value <= high0) || (value >= low1 && value <= high1) || (value >= low2 && value <= high2) || (value >= low3 && value <= high3));
}

vector<tuple<Vec4i, Vec4i, double>> CubeDetector::getAllPairs(vector<pair<Vec4i, bool>>& lines_A, vector<pair<Vec4i, bool>>& lines_B) {
	vector<tuple<Vec4i, Vec4i, double>> all_pairs;
	for (pair<Vec4i, bool>& pair_a : lines_A) {
		Vec4i& a = pair_a.first;
		double length_A = length(a[0], a[1], a[2], a[3]);
		for (pair<Vec4i, bool>& pair_b : lines_B) {
			Vec4i& b = pair_b.first;
			double length_B = length(b[0], b[1], b[2], b[3]);
			if (abs(length_A - length_B) <= 15) {
				all_pairs.push_back(make_tuple(a, b, length(a[0], a[1], a[2], a[3]) + length(b[0], b[1], b[2], b[3])));
			}
		}
	}
	return all_pairs;
}

double CubeDetector::getLongest(vector<pair<Vec4i, bool>>& lines) {
	double longest = 0;
	for (pair<Vec4i, bool>& p : lines) {
		Vec4i& l = p.first;
		longest = max(longest, length(l[0], l[1], l[2], l[3]));
	}
	return longest;
}

template<class T> void CubeDetector::drawLines(vector<pair<Vec4i, T>>& angles, Mat& mat) {
	for (const pair<Vec4i, T>& a : angles) {
		Vec4i l = a.first;
		line(mat, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,255,0), 1, CV_AA);
	}
}

void CubeDetector::filterLines(vector<pair<Vec4i, double>>& angles, const double& low_angle, const double& high_angle, vector<pair<Vec4i, bool>>& lines_A, vector<pair<Vec4i, bool>>& lines_B) {
	double a1 = low_angle - 3*CV_PI/80;
	double a2 = low_angle + 3*CV_PI/80;
	double a3 = high_angle - 3*CV_PI/80;
	double a4 = high_angle + 3*CV_PI/80;
	for (int i = 0; i < angles.size(); i++) {
		double angle = angles[i].second;
		if (angle >= a1 && angle <= a2) {
			lines_A.push_back(make_pair(angles[i].first, false));
		} else if (angle >= a3 && angle <= a4) {
			lines_B.push_back(make_pair(angles[i].first, false));
		} else {
			angles.erase(angles.begin() + i);
			i--;
		}
	}
}

int CubeDetector::findCubeAngles(const vector<int>& angle_count, double& low_angle, double& high_angle) {
	int max = 0;
	low_angle = -INF;
	high_angle = -INF;
	for (int i = 0; i < 2*angle_count.size()/3; i++) {
		for (int j = i-angle_count.size()/3; j >= 0; j--) {
			int current = angle_count[i] + angle_count[j];
			if (current > max) {
				max = current;
				low_angle = i;
				high_angle = j;
			}
		}
		for (int j = i+angle_count.size()/3; j < angle_count.size(); j++) {
			int current = angle_count[i] + angle_count[j];
			if (current > max) {
				max = current;
				low_angle = i;
				high_angle = j;
			}
		}
	}
	if (low_angle == -INF) {
		return -1;
	}
	if (high_angle < low_angle) {
		double temp = high_angle;
		high_angle = low_angle;
		low_angle = temp;
	}
	low_angle = (2*low_angle + 1)*CV_PI/(2*angle_count.size()) - CV_PI/2;
	high_angle = (2*high_angle + 1)*CV_PI/(2*angle_count.size()) - CV_PI/2;
	return max;
}

void CubeDetector::removeSmallAngles(vector<pair<Vec4i, double>>& angles) {
	for (int i = 0; i < angles.size();) {
		double angle = angles[i].second;
		if (angle > -CV_PI/6 && angle < CV_PI/6) {
			double limit = angle - CV_PI/3;
			int j = 0;
			bool keep = false;
			while (j < angles.size() && angles[j].second <= limit) {
				if (intersect(angles[i].first, angles[j].first)) {
					keep = true;
					i++;
					break;
				}
				j++;
			}
			limit = angle + CV_PI/3;
			j = angles.size() - 1;
			if (!keep) {
				while (j >= 0 && angles[j].second >= limit) {
					if (intersect(angles[i].first, angles[j].first)) {
						keep = true;
						i++;
						break;
					}
					j--;
				}
			}
			if (!keep) {
				angles.erase(angles.begin() + i);
			}
		} else {
			double low;
			double high;
			if (angle <= -CV_PI/6) {
				low = angle + CV_PI/3;
				high = angle + 2*CV_PI/3;
			} else {
				low = angle - 2*CV_PI/3;
				high = angle - CV_PI/3;
			}
			int j = 0;
			bool keep = false;
			while (j < angles.size() && angles[j].second < low) {
				j++;
			}
			while (j < angles.size() && angles[j].second <= high) {
				if (intersect(angles[i].first, angles[j].first)) {
					keep = true;
					i++;
					break;
				}
				j++;
			}
			if (!keep) {
				angles.erase(angles.begin() + i);
			}
		}
	}
}

void CubeDetector::createGrid(set<Vec4i*>& cycle) {
	vector<Vec4i*> lines;
	for (Vec4i* line : cycle) {
		lines.push_back(line);
	}
	sort(lines.begin(), lines.end(), &CubeDetector::gradientComparator);
	addDivisions(lines, 0, cycle);
	addDivisions(lines, 2, cycle);
}

void CubeDetector::addDivisions(vector<Vec4i*>& lines, int start_index, set<Vec4i*>& cycle) {
	int first = start_index;
	int second = first+1;
	if ((*(lines[first]))[0] > (*(lines[second]))[0]) {
		Vec4i* temp = lines[first];
		lines[first] = lines[second];
		lines[second] = temp;
	}
	Vec4i& l1 = *(lines[first]);
	Vec4i& l2 = *(lines[second]);
	cycle.insert(new Vec4i((l1[0] + 2*l1[2])/3, (l1[1] + 2*l1[3])/3, (l2[0] + 2*l2[2])/3, (l2[1] + 2*l2[3])/3));
	cycle.insert(new Vec4i((2*l1[0] + l1[2])/3, (2*l1[1] + l1[3])/3, (2*l2[0] + l2[2])/3, (2*l2[1] + l2[3])/3));
}

void CubeDetector::fitPoints(set<Vec4i*>& cycle, Point& centre) {
	int radius = 0;
	for (Vec4i* line : cycle) {
		radius += length(centre.x, centre.y, (*line)[0], (*line)[1]);
	}
	radius /= 4;
	for (Vec4i* line : cycle) {
		int distance = length(centre.x, centre.y, (*line)[0], (*line)[1]);
		int dx = centre.x - (*line)[0];
		int dy = centre.y - (*line)[1];
		(*line)[0] = centre.x - dx*radius/distance;
		(*line)[1] = centre.y - dy*radius/distance;
		distance = length(centre.x, centre.y, (*line)[2], (*line)[3]);
		dx = centre.x - (*line)[2];
		dy = centre.y - (*line)[3];
		(*line)[2] = centre.x - dx*radius/distance;
		(*line)[3] = centre.y - dy*radius/distance;
	}
}

Point CubeDetector::findMean(set<Vec4i*> lines_end_points) {
	if (lines_end_points.empty()) {
		return Point(0, 0);
	}
	int x = 0;
	int y = 0;
	for (auto it = lines_end_points.begin(); it != lines_end_points.end(); it++) {
		Vec4i& line = **it;
		x += line[0] + line[2];
		y += line[1] + line[3];
	}
	return Point(x/(2*lines_end_points.size()), y/(2*lines_end_points.size()));
}

Point CubeDetector::findMedian(set<Vec4i*> lines_end_points) {
	if (lines_end_points.empty()) {
		return Point(0, 0);
	}
	vector<int> xs;
	vector<int> ys;
	for (auto it = lines_end_points.begin(); it != lines_end_points.end(); it++) {
		Vec4i& line = **it;
		xs.push_back(line[0] + line[2]);
		ys.push_back(line[1] + line[3]);
	}
	sort(xs.begin(), xs.end());
	sort(ys.begin(), ys.end());
	return Point(xs[xs.size()/2]/2, ys[ys.size()/2]/2);
}

void CubeDetector::joinPoints(vector<pair<Vec4i, bool>>& linesA, vector<pair<Vec4i, bool>>& linesB) {
	for (int i = 0; i < linesA.size(); i++) {
		Vec4i& line = linesA[i].first;
		for (int j = 0; j < linesB.size(); j++) {
			if (i != j) {
				bool merged = false;
				Vec4i& current = linesB[j].first;
				merged = mergePoints(line[0], line[1], current[0], current[1]);
				merged |= mergePoints(line[0], line[1], current[2], current[3]);
				merged |= mergePoints(line[2], line[3], current[0], current[1]);
				merged |= mergePoints(line[2], line[3], current[2], current[3]);
				if (!merged) {
					minDistance(line, current, true, 5);
					merged = true;
				}
				if (merged) {
					linesA[i].second = true;
					linesB[j].second = true;
				}
			}
		}
	}
	removeFalse(linesA);
	removeFalse(linesB);
}

void CubeDetector::removeFalse(vector<pair<Vec4i, bool>>& lines) {
	for (int i = lines.size()-1; i >= 0; i--) {
		if (!lines[i].second) {
			lines.erase(lines.begin() + i);
		}
	}
}

bool CubeDetector::mergePoints(int& x1, int& y1, int& x2, int& y2) {
	if (length(x1, y1, x2, y2) < distance_threshold) {
		int x = (x1+x2)/2;
		int y = (y1+y2)/2;
		x1 = x;
		y1 = y;
		x2 = x;
		y2 = y;
		return true;
	}
	return false;
}

vector<Point> CubeDetector::findIntersections(const vector<pair<Vec4i, double>>& linesA, const vector<pair<Vec4i, double>>& linesB) {
	vector<Point> intersections;
	for (int i = 0; i < linesA.size(); i++) {
		Vec4i line = linesA[i].first;
		float gradientA = ((float)(line[3]-line[1]))/(line[2]-line[0]);
		float intersectA = line[1] - gradientA*line[0];
		for (int j = 0; j < linesB.size(); j++) {
			Vec4i current = linesB[j].first;
			float gradientB = ((float)(current[3]-current[1]))/(current[2]-current[0]);
			float intersectB = current[1] - gradientB*current[0];
			float x = (intersectA - intersectB)/(gradientB - gradientA);
			int y = gradientA*x + intersectA;
			intersections.push_back(Point((int) x, y));
		}
	}
	return intersections;
}

void CubeDetector::mergeLines(vector<pair<Vec4i, bool>>& lines) {
	int size = lines.size();
	for (int i = 0; i < size; i++) {
		Vec4i& line = lines[i].first;
		int stop = i;
		for (int j = i+1; j != stop && j < lines.size();) {
			Vec4i current = lines[j].first;
			if (intersect(line, current) || minDistance(line, current) < 5) {
				double a = length(line[0],line[1],line[2],line[3]);
				double b = length(current[0],current[1],current[2],current[3]);
				double c = length(line[0],line[1],current[2],current[3]);
				double d = length(current[0],current[1],line[2],line[3]);
				int index = 0;
				double max = a;
				if (b >= max) {
					max = b;
					index = 1;
				}
				if (c >= max) {
					max = c;
					index = 2;
				}
				if (d >= max) {
					max = d;
					index = 3;
				}
				if (max == 1) {
					line[0] = current[0];
					line[1] = current[1];
					line[2] = current[2];
					line[3] = current[3];
				} else if (max == 2) {
					line[2] = current[2];
					line[3] = current[3];
				} else if (max == 3) {
					line[0] = current[0];
					line[0] = current[1];
				}
				lines.erase(lines.begin()+j);
				size--;
				if (size == 1) {
					break;
				}
				if (j < stop) {
					stop--;
				}
				j = j%size;
			} else {
				j = (j+1)%size;
			}
		}
	}
}

bool CubeDetector::pointComparator(const pair<Vec4i, double>& a, const pair<Vec4i, double>& b) {
	if (a.second < b.second) {
		return 1;
	} else if (a.second > b.second) {
		return 0;
	} else if (a.first[0] < b.first[0]) {
		return 1;
	} else if (a.first[0] > b.first[0]) {
		return 0;
	} else {
		return a.first[1] < b.first[1];
	}
}

bool CubeDetector::gradientComparator(Vec4i* a, Vec4i* b) {
	double grad_a = gradient(a);
	double grad_b = gradient(b);
	if (grad_a < grad_b) {
		return 1;
	} else if (grad_a > grad_b) {
		return 0;
	} else if ((*a)[0] < (*b)[0]) {
		return 1;
	} else if ((*a)[0] > (*b)[0]) {
		return 0;
	} else {
		return (*a)[1] < (*b)[1];
	}
}

bool CubeDetector::pairsLengthComparator(tuple<Vec4i, Vec4i, double>& a, tuple<Vec4i, Vec4i, double>& b) {
	if (get<2>(a) <= get<2>(b)) {
		return 0;
	} else {
		return 1;
	}
}

bool CubeDetector::hueComparator(Scalar& a, Scalar& b) {
	return a[0] < b[0];
}