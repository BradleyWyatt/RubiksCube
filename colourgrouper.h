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

#include "circular.h"

class ColourGrouper {

private:

	Circular circular;
	vector<pair<pair<int, int>, pair<int, int>>> adjacencies;
	double saturation_threshold_high;
	double saturation_threshold_low;
	double saturation_value_difference;
	
	// Attempts to complete the remaining empty positions based on constraints alone (not colour value)
	bool fitConstraints(int positions[6][9]);

	// Adds a single adjacency between (a,b) and (c,d)
	void addAdjacency(int a, int b, int c, int d);

	// Generates adjacency list
	void generateAdjacencies();

	// Checks whether the colours of the cube meet the constraints
	bool checkConstraints(int positions[6][9]);

	// Updates the centre to be the mean of colours
	void updateCentre(Scalar& centre, Scalar colours[9]);

	// Finds the difference of values from scalar and stores in differences
	void updateDifferences(Scalar& centre, Scalar values[9], double differences[9]);

	// Updates the centre to be the mean of colours in Euclidean space
	void updateCentre2(Scalar& centre, Scalar colours[9]);

	// Finds the difference of values from scalar and stores in differences in Euclidean space
	void updateDifferences2(Scalar& centre, Scalar values[9], double differences[9]);

public:

	ColourGrouper();

	// Attempts to fit colours so that they match the constraints
	bool fitColours(int positions[6][9], Scalar values[6][9], Scalar centres[6]);

	// Group the colours together and write to image
	void groupColours(Mat& image, Scalar values[][9], int positions[6][9], Scalar centres[6], int white_side);

	// Group the colours assuming Euclidean distance
	void groupColours(Mat& image, Scalar values[][9], int positions[6][9], Scalar centres[6], bool is_hsv);

};