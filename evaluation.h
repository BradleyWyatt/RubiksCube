#pragma once

#include <opencv2/core/core.hpp>
#include <opencv2/core/operations.hpp>

using namespace cv;
using namespace std;

namespace evaluation {

	// Generates command line for random colour balance
	void generateFFmpeg();

	// Processes file data
	void processData();

	// Evalation for solver
	void solver(int num_trials, bool solvable);

	// Evalation for colour grouping
	void colours(Scalar values[6][9], string id);

	// Evalation for position detection
	void position();

}