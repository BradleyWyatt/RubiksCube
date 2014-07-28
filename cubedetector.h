#pragma once

#include <cmath>
#include <iostream>
#include <mutex>
#include <set>
#include <tuple>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/core/operations.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>

using namespace cv;
using namespace std;

class CubeDetector {

private:

	int height;
	int width;
	Size size;
	//string name;
	string input_cube;
	string detected_cube;
	string cube_grid;
	vector<Mat> previous_frames;
	Scalar hsv_values[6][9];
	Scalar bgr_values[6][9];
	int prev_frames;
	int angle_threshold;
	int distance_threshold;
	int angle_count_size;
	bool complete;
	double saturation_value_difference;

	Mat intrinsic;
	Mat dist_coeffs;

	mutex last_detected_mutex;
	mutex face_index_mutex;
	mutex grid_obtained_mutex;
	mutex calibrated_mutex;

	vector<vector<Point>> last_detected_grid;
	Mat last_detected_cube;
	bool grid_obtained;
	int face_index_global;
	bool calibrated;
	
	Scalar skin_mean;
	Scalar skin_diff;
	Scalar skin_stdev;
	double total_time;
	int moves;
	int skin_detect_moves;

	int ksize_slider;
	int gsize_slider;
	int sigma_slider;
	int rho_slider;
	int theta_slider;
	int val_slider;
	int threshold_slider;
	int length_slider; // Set to 25/30 if height = 200/400
	int gap_slider;
	int morph_slider;
	int angle_slider;
	int numlines_slider;
	int low;
	int high;

	// To be run by a thread. Finds the location of the cube in the videoCapture
	void captureCube(VideoCapture& capture);
	
	// Undistorts the input image
	Mat getUndistorted(Mat &image);

	// Orientates the grid so index 0 is top left, counting horizontally, and index 8 is bottom right
	vector<vector<Point>> orientateGrid(vector<vector<Point>> grid, int index);

	// Finds the gradient of grid
	double findGradient(const vector<vector<Point>>& grid);

	// Flips the grid along a horizontal axis
	void flipGridHor(vector<vector<Point>>& grid);

	// Flips the grid along an vertical axis
	void flipGridVert(vector<vector<Point>>& grid);

	// Rotates grid clockwise by 90*rotations degrees 
	void rotateGrid(vector<vector<Point>>& grid, int rotations);

	// Finds the index of the square with the lowest x co-ordinate
	int findLowestXindex(const vector<vector<Point>>& grid);

	// Finds the index of the square with the lowest y co-ordinate
	int findLowestYindex(const vector<vector<Point>>& grid);

	// If key_press = 32 (space bar) updates the face with index face_index onto image cube with colours from input_frame using points in grid to create a mask
	bool checkCubeStatus(int key_press, int& face_index, bool& grid_obtained, Mat& cube, Mat& input_frame, const vector<vector<Point>>& grid);

	// Takes input frame and an average total frame for background subtraction. prev_colour contains the previous frames, which is to be updated at index count to calculate the new total
	Mat processFrame(const Mat& frame, Mat& scaled, Mat& total, vector<Mat> prev_colour, int count);

	// Draws grid lines parallel to line_A, which intersect line_B onto mat
	void drawGridLines(const Vec4i& line_A, const Vec4i& line_B, double intersect_double, Mat& mat);

	// Creates 4 points around each of the stickers on the face of the cube
	void addToGrid(const Vec4i& line_A, const Vec4i& line_B, double& intersect_double_A, double& intersect_double_B, double& x, double& y, vector<vector<Point>>& grid);

	// Returns true of value is within a threshold of the values 0, 1, 2 or 3.
	bool inRange(double value);

	// Finds the centre piece with the maximum difference between Saturation and Value
	int findMaxSVdiff(Scalar values[][9], double& max_difference);

	// If there is a white side of the cube, returns the face index of it
	int findWhiteSide(Scalar hsv_values[][9]);

	// Finds the 3x3 Rubik's Cube. Returns a vector of a vector defining the points of the corners of each sticker
	vector<vector<Point>> findGrid(vector<tuple<Vec4i, Vec4i, double>>& all_pairs, vector<pair<Vec4i, bool>>& lines_A, vector<pair<Vec4i, bool>>& lines_B, Mat& mat);

	// Takes cartesian product of the vectors of Vec4i's and filters out lines which are not of similar length.
	vector<tuple<Vec4i, Vec4i, double>> getAllPairs(vector<pair<Vec4i, bool>>& lines_A, vector<pair<Vec4i, bool>>& lines_B);

	// Returns the length of the longest line in the vector
	double getLongest(vector<pair<Vec4i, bool>>& lines);

	// Draws the lines represented by Vec4i on Mat mat
	template<class T> void drawLines(vector<pair<Vec4i, T>>& lines, Mat& mat);

	// Copies lines from angles which are near low_angle to lines_A, and angles near high_angle to lines_B
	void filterLines(vector<pair<Vec4i, double>>& angles, const double& low_angle, const double& high_angle, vector<pair<Vec4i, bool>>& lines_A, vector<pair<Vec4i, bool>>& lines_B);

	// Finds the angle of each line and stores in a pair alongside original line
	vector<pair<Vec4i, double>> calcAngles(const vector<Vec4i>& lines);

	// Splits 2pi range into num_sections sections and counts the number of angles in each section
	vector<int> countAngles(vector<pair<Vec4i, double>> angles, int num_sections);

	// Finds the pair of angles which differ by at least 60 degrees, and have the most lines at those angles.
	// Returns the number of those  lines, and gives the angles in low_angle and high_angle
	int findCubeAngles(const vector<int>& angle_count, double& low_angle, double& high_angle);

	// Creates the masks to be used to extract the colour of the stickers from scaled
	void createStickerMasks(Mat& scaled, int& face_index, int num_lines, vector<pair<Vec4i, double>>& angles, int low_angle, int high_angle, Mat& cube_grid, bool& grid_obtained, vector<vector<Point>>& grid, Mat& cube);

	// Removes all lines which don't have intersects, or intersect at an angle < 60
	void removeSmallAngles(vector<pair<Vec4i, double>>& angles);

	// Creates a 3x3 from the square given in cycle
	void createGrid(set<Vec4i*>& cycle);

	// Adds Vec4i*s to cycle which are joined at 1/3s of the way along lines at index start_index and start_index+1
	void addDivisions(vector<Vec4i*>& lines, int start_index, set<Vec4i*>& cycle);

	// Fits the points of cycle to a circle centred on centre
	void fitPoints(set<Vec4i*>& cycle, Point& centre);

	// Finds the median centre point of all the lines in the set
	Point findMedian(set<Vec4i*> lines_end_points);

	// Finds the mean centre point of all the lines in the set
	Point findMean(set<Vec4i*> lines_end_points);

	// Removes pairs which have a second element of false
	void removeFalse(vector<pair<Vec4i, bool>>& lines);

	// Returns a set of points which are the end points of at least 2 different lines
	void joinPoints(vector<pair<Vec4i, bool>>& linesA, vector<pair<Vec4i, bool>>& linesB);

	// Returns a vector of points of intersections of the lines between sets A and B
	vector<Point> findIntersections(const vector<pair<Vec4i, double>>& linesA, const vector<pair<Vec4i, double>>& linesB);

	// Merges lines which are close together, possibly crossing and have a similar angle
	void mergeLines(vector<pair<Vec4i, bool>>& lines);

	// If points are within a threshold distance of one another, set them as the same coordinate and return true
	bool mergePoints(int& x1, int& y1, int& x2, int& y2);

	// Sorts lines by angle, then by x, then by y
	static bool pointComparator(const pair<Vec4i, double>& a, const pair<Vec4i, double>& b);

	// Sorts Vec4i* by gradient, then by x, then by y
	static bool gradientComparator(Vec4i* a, Vec4i* b);

	// Sorts by the length of the sum of the vectors in tuple in descending order
	static bool pairsLengthComparator(tuple<Vec4i, Vec4i, double>& a, tuple<Vec4i, Vec4i, double>& b);

	// Sorts by the length of the sum of the vectors in tuple in descending order
	bool hueComparator(Scalar& a, Scalar& b);

public:

	CubeDetector();

	// Runs the entire program
	void startDetection();

	// Checks if side 0 of the cube is as expected
	void checkColours(int positions[6][9], Scalar colours[6], vector<int> updated);

};