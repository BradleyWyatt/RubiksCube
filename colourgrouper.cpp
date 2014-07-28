#include "stdafx.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include <Windows.h>

#include <opencv2/core/core.hpp>
#include <opencv2/core/operations.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/video/tracking.hpp>

#include "circular.h"
#include "cube.h"
#include "cubesolver.h"
#include "colourgrouper.h"
#include "util.h"

#define INF 999999

ColourGrouper::ColourGrouper() {
	saturation_threshold_high = 140;
	saturation_threshold_low = 80;
	saturation_value_difference = 50;
}

bool ColourGrouper::fitConstraints(int positions[6][9]) {
	if (adjacencies.empty()) {
		generateAdjacencies();
	}
	vector<pair<int,int>> empty_positions;
	int counters[6] = {0,0,0,0,0,0};
	for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 9; j++) {
			if (positions[i][j] == -1) {
				empty_positions.push_back(make_pair(i,j));
			} else {
				counters[positions[i][j]]++;
			}
		}
	}
	for (int i = 0; i < 6; i++) {
	///	cout << counters[i] << endl;
	}
	//cout << "Empty positions size" << endl;
	//cout << empty_positions.size() << endl;
	for (int i = 0; i < empty_positions.size(); i++) {
		bool possible[6];
		for (int j = 0; j < 6; j++) {
			if (counters[j] != 9) {
				possible[j] = true;
			} else {
				possible[j] = false;
			}
		}
		for (pair<pair<int,int>,pair<int,int>> adjacency : adjacencies) {
			//cout << i << " " << empty_positions[i].first << " " << empty_positions[i].second << " " << adjacency.first.first << " " << adjacency.first.second << " " << adjacency.second.first << " " << adjacency.second.second << endl;
			if (adjacency.first.first == empty_positions[i].first && adjacency.first.second == empty_positions[i].second) {
				if (positions[adjacency.second.first][adjacency.second.second] != -1) {
					possible[positions[adjacency.second.first][adjacency.second.second]] = false;
				}
			} else if (adjacency.second.first == empty_positions[i].first && adjacency.second.second == empty_positions[i].second) {
				if (positions[adjacency.second.first][adjacency.second.second] != -1) {
					possible[positions[adjacency.first.first][adjacency.first.second]] = false;
				}
			}
		}
		int possiblities = 0;
		int last = -1;
		for (int j = 0; j < 6; j++) {
			if (possible[j] == true) {
				possiblities++;
				last = j;
			}
		}
		//cout << i << " " << empty_positions[i].first << " " << empty_positions[i].second << " " << possiblities << " " << last << endl;
		if (possiblities == 1) {
			positions[empty_positions[i].first][empty_positions[i].second] = last;
			counters[last]++;
			empty_positions.erase(empty_positions.begin()+i, empty_positions.begin()+i+1);
			i = 0;
		}
	}
	vector<int> remaining_values;
	for (int i = 0; i < 6; i++) {
		//cout << counters[i] << endl;
		for (int j = 0; j < 9-counters[i]; j++) {
			remaining_values.push_back(i);
		}
	}
	if (remaining_values.empty()) {
		return true;
	}
	vector<pair<int,int>> non_placed;
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 9; j++) {
			if (positions[i][j] == -1) {
				non_placed.push_back(make_pair(i,j));
			}
		}
	}
	bool complete = false;
	//cout << "Remaining values size" << endl;
	//cout << remaining_values.size() << endl;
	if (remaining_values.size() > 7) {
		return false;
	}
	do {
		for (int i = 0; i < remaining_values.size(); i++) {
			//cout << "Assigning " << non_placed[i].first << ", " << non_placed[i].second << " colour " << remaining_values[i] << endl;
			positions[non_placed[i].first][non_placed[i].second] = remaining_values[i];
		}
		Cube cube(positions);
		CubeSolver cube_solver(&cube);
		if (checkConstraints(positions) && cube_solver.solve()) {
			complete = true;
			break;
		}
	} while (next_permutation(remaining_values.begin(), remaining_values.end()));
	return complete;
}

bool ColourGrouper::fitColours(int positions[6][9], Scalar values[6][9], Scalar centres[6]) {
	int differences[6][9];
	bool placed[6][9];
	int positions_copy[6][9];
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 9; j++) {
			if (j != 4) {
				Scalar centre = centres[positions[i][j]];
				differences[i][j] = abs(values[i][j][0]-centre[0]) + abs(values[i][j][1]-centre[1]) + abs(values[i][j][2]-centre[2]);
				placed[i][j] = false;
				positions_copy[i][j] = positions[i][j];
				positions[i][j] = -1;
			}
		}
	}
	bool complete = true;
	for (int i = 0; i < 48; i++) {
		pair<int,int> min_index = make_pair(-1,-1);
		int min_difference = INF;
		for (int j = 0; j < 6; j++) {
			for (int k = 0; k < 9; k++) {
				if (k != 4 && !placed[j][k] && differences[j][k] < min_difference) {
					min_difference = differences[j][k];
					min_index = make_pair(j,k);
				}
			}
		}
		positions[min_index.first][min_index.second] = positions_copy[min_index.first][min_index.second];
		placed[min_index.first][min_index.second] = true;
		if (!checkConstraints(positions)) {
		///	cout << "Failed " << min_index.first << " " << min_index.second << endl;
			positions[min_index.first][min_index.second] = -1;
			complete = false;
		}
	}
	if (!complete) {
		return fitConstraints(positions);
	} else {
		return true;
	}
}

void ColourGrouper::addAdjacency(int a, int b, int c, int d) {
	adjacencies.push_back(make_pair(make_pair(a, b), make_pair(c, d)));
}

void ColourGrouper::generateAdjacencies() {
	for (int i = 0; i < 4; i+=2) {
		for (int j = 0; j < 9; j++) {
			if (j%3 == 0) {
				addAdjacency(i, j, (i+3)%4, j+2);
			} else if ((j+1)%3 == 0) {
				addAdjacency(i, j, (i+1)%4, j-2);
			}
		}
	}
	addAdjacency(0,0,4,8);
	addAdjacency(0,1,4,5);
	addAdjacency(0,2,4,2);

	addAdjacency(0,6,5,2);
	addAdjacency(0,7,5,5);
	addAdjacency(0,8,5,8);

	addAdjacency(1,0,4,2);
	addAdjacency(1,1,4,1);
	addAdjacency(1,2,4,0);

	addAdjacency(1,6,5,8);
	addAdjacency(1,7,5,7);
	addAdjacency(1,8,5,6);

	addAdjacency(2,0,4,0);
	addAdjacency(2,1,4,3);
	addAdjacency(2,2,4,6);

	addAdjacency(2,6,5,6);
	addAdjacency(2,7,5,3);
	addAdjacency(2,8,5,0);

	addAdjacency(3,0,4,6);
	addAdjacency(3,1,4,7);
	addAdjacency(3,2,4,8);

	addAdjacency(3,6,5,0);
	addAdjacency(3,7,5,1);
	addAdjacency(3,8,5,2);
}

bool ColourGrouper::checkConstraints(int positions[6][9]) {
	if (!adjacencies.empty()) {
		generateAdjacencies();
	}
	pair<int, int> opposites[3];
	opposites[0] = make_pair(positions[0][4], positions[2][4]);
	opposites[1] = make_pair(positions[1][4], positions[3][4]);
	opposites[2] = make_pair(positions[4][4], positions[5][4]);
	for (pair<pair<int,int>,pair<int,int>> adjacency : adjacencies) {
		int a = adjacency.first.first;
		int b = adjacency.first.second;
		int c = adjacency.second.first;
		int d = adjacency.second.second;
		if (positions[a][b] == positions[c][d]) {
			if (positions[a][b] != -1 && positions[c][d] != -1) {
			//	cout << adjacency.first.first << " " << adjacency.first.second << " " << adjacency.second.first << " " << adjacency.second.second << endl;
				return false;
			}
		}
		for (pair<int, int> opposite : opposites) {
			if (positions[a][b] == opposite.first && positions[c][d] == opposite.second || positions[a][b] == opposite.second && positions[c][d] == opposite.first) {
				if (positions[a][b] != -1 && positions[c][d] != -1) {
			//		cout << positions[a][b] << " " << positions[c][d] << " are adjacent on positions " << a << " " << b << " " << c << " " << d << endl;
					return false;
				}
			}
		}
	}
	int counters[6] = {0,0,0,0,0,0};
	for (int i = 0; i < 6; i++) {
		for (int j = 1; j < 9; j+=2) {
			if (positions[i][j] != -1) {
				counters[positions[i][j]]++;
			}
		}
	}
	for (int i = 0; i < 6; i++) {
		if (counters[i] > 4) {
		//	cout << "Edge Counter " << i << " = " << counters[i] << endl;
			return false;
		}
	}
	for (int i = 0; i < 6; i++) {
		counters[i] = 0;
	}
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 9; j+=2) {
			if (j != 4) {
				if (positions[i][j] != -1) {
					counters[positions[i][j]]++;
				}
			}
		}
	}
	for (int i = 0; i < 6; i++) {
		if (counters[i] > 4) {
		//	cout << "Corner Counter " << i << " = " << counters[i] << endl;
			return false;
		}
	}
	return true;
}

void ColourGrouper::groupColours(Mat& image, Scalar values[][9], int positions[6][9], Scalar centres[6], int white_side) {
	vector<int> random;
	for (int i = 0; i < 54; i++) {
		if (i % 9 != 4) {
			random.push_back(i);
		}
	}
	srand(GetTickCount());
	random_shuffle(random.begin(), random.end());
	Scalar temp_values[6][9];
	pair<int, int> cube_indices[6][9];
	pair<int, int> temp_cube_indices[6][9];
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 9; j++) {
			temp_values[i][j] = values[i][j];
			temp_cube_indices[i][j] = make_pair(i,j);
		}
	}
	int random_index = 0;
	for (int i = 0; i < 48; i++) {
		if (random_index%9 == 4) {
			random_index++;
		}
		int r = random[i];
		values[random_index/9][random_index%9] = temp_values[r/9][r%9];
		cube_indices[random_index/9][random_index%9] = temp_cube_indices[r/9][r%9];
		random_index++;
	}
	int starting_centres[6];
	for (int i = 0; i < 6; i++) {
		starting_centres[i] = i;
		updateCentre(centres[i], values[i]);
	}
	double hue_differences[6][10];
	for (int i = 0; i < 6; i++) {
		Scalar centre = centres[i];
		double total_hue_difference = 0;
		cube_indices[i][4] = make_pair(i, 4);
		for (int j = 0; j < 9; j++) {
			Scalar current = values[i][j];
			hue_differences[i][j] = min(abs(current[0]-centre[0]), abs(abs(current[0]-centre[0])-180));
			total_hue_difference += hue_differences[i][j];
		}
		hue_differences[i][9] = total_hue_difference;
	}
	bool change = false;
	int count = 0;
	int cycle = 0;
	int changes = 0;
	while (cycle < 3) {
		change = false;
		count++; // Debug code. Delete
		for (int i = 0; i < 6; i++) {
			if (cycle == 2 && white_side == i) {
				continue;
			}
			for (int j = 0; j < 9; j++) {
				double hue = values[i][j][0];
				double saturation = values[i][j][1];
				double value = values[i][j][2];
				for (int k = 0; k < 6; k++) {
					if (i == k || (cycle == 2 && white_side == k)) {
						continue;
					}
					Scalar* current_group = values[k];
					for (int l = 0; l < 9; l++) {
						if ((j == 4 && l == 4) || (j != 4 && l != 4)) {
							if (cycle == 0 || cycle == 2) {
								if (saturation > saturation_threshold_low) {
									double current_hue = current_group[l][0];
									// Switch the colours
									Scalar temp = values[i][j];
									values[i][j] = values[k][l];
									values[k][l] = temp;
									Scalar colour_A;
									Scalar colour_B;
									updateCentre(colour_A, values[i]);
									updateCentre(colour_B, values[k]);
									double differences_A[10];
									double differences_B[10];
									updateDifferences(colour_A, values[i], differences_A);
									updateDifferences(colour_B, values[k], differences_B);
									//	cout << differences_A[9] << " " << differences_B[9] << " " << hue_differences[i][9] << " " << hue_differences[k][9] << endl;
									if (differences_A[9] + differences_B[9] < hue_differences[i][9] + hue_differences[k][9]) {
										changes++;
										for (int a = 0; a < 10; a++) {
											hue_differences[i][a] = differences_A[a];
											hue_differences[k][a] = differences_B[a];
										}
										pair<int,int> cube_index = cube_indices[i][j];
										cube_indices[i][j] = cube_indices[k][l];
										cube_indices[k][l] = cube_index;
										centres[i] = colour_A;
										centres[k] = colour_B;
										change = true;
										// cout << difference_A << " " << difference_B << " " << hue_differences[i][j] << " " << hue_differences[k][l] << endl;
										if (j == 4) {
											if (i == white_side) {
										///		cout << "Changing white side from " << starting_centres[i] << " to " << starting_centres[k] << endl;
											} else if (k == white_side) {
										///		cout << "Changing white side from " << starting_centres[k] << " to " << starting_centres[i] << endl;
											}
											int starting_index = starting_centres[i];
											starting_centres[i] = starting_centres[k];
											starting_centres[k] = starting_index;
										}
										//cout << i << " " << j << " " << k << " " << l << endl;
										// To break out of the 4 loops:
										i = 6;
										j = 9;
										k = 6;
										l = 9;
										break;
									} else {
										// Switch the colours back
										Scalar temp = values[i][j];
										values[i][j] = values[k][l];
										values[k][l] = temp;
									}
								}
							} else if (cycle == 1) {
								if (white_side == k && value-saturation > saturation_value_difference) {
									Scalar white_hsv = values[k][l];
									Scalar temp = values[i][j];
									values[i][j] = values[k][l];
									values[k][l] = temp;
									Scalar colour_A;
									Scalar colour_B;
									updateCentre(colour_A, values[i]);
									updateCentre(colour_B, values[k]);
									// If there is a saturation over the threshold on the predicted white stickers, and the saturation of the current is lower, and the hue is similar
									//cout << saturation-value  << " " <<  abs(value-colour_B[2])  << " " <<  abs(white_hsv[2]-colour_A[2])  << " ";
									//cout << white_hsv[1] - white_hsv[2]  << " " <<  abs(value-centres[i][2])  << " " <<  abs(white_hsv[2]-centres[k][2]) << " " << endl;
									if (saturation-value + abs(value-colour_B[2]) + abs(white_hsv[2]-colour_A[2])
										+ min(abs(hue-colour_B[0]), abs(abs(hue-colour_B[0])-180)) + min(abs(white_hsv[0]-colour_A[0]), abs(abs(white_hsv[0]-colour_A[0])-180))
										< white_hsv[1] - white_hsv[2] + abs(value-centres[i][2]) + abs(white_hsv[2]-centres[k][2])
										+ min(abs(hue-centres[i][0]), abs(abs(hue-centres[i][0])-180)) + min(abs(white_hsv[0]-centres[k][0]), abs(abs(white_hsv[0]-centres[k][0])-180))) {
										//cout << values[i][j] << " " << values[white_side][index] << endl;
										//cout << saturation << endl;
										//cout << min(abs(hue-centres[white_side][0]), abs(abs(hue-centres[white_side][0])-180)) << " " <<  min(abs(hue-centres[i][0]), abs(abs(hue-centres[i][0])-180)) << endl;
										changes++;
										pair<int,int> cube_index = cube_indices[i][j];
										cube_indices[i][j] = cube_indices[white_side][l];
										cube_indices[white_side][l] = cube_index;
										centres[i] = colour_A;
										centres[white_side] = colour_B;
										updateDifferences(centres[i], values[i], hue_differences[i]);
										updateDifferences(centres[white_side], values[white_side], hue_differences[5]);
										if (j == 4) {
										///	cout << "Changing white side from " << starting_centres[k] << " to " << starting_centres[i] << endl;
											int starting_index = starting_centres[i];
											starting_centres[i] = starting_centres[k];
											starting_centres[k] = starting_index;
										}
									///	cout << cube_indices[i][j].first << " " << cube_indices[i][j].second << " " << cube_indices[k][l].first << " " << cube_indices[k][l].second << endl;
										change = true;
										i = 6;
										j = 9;
										k = 6;
										l = 9;
										break;
									} else {
										// Switch the colours back
										Scalar temp = values[i][j];
										values[i][j] = values[k][l];
										values[k][l] = temp;
									}
								}
							}
						}
					}
				}
			}
		}
		if (changes > 500) {
			change = false;
		}
		if (!change) {
	//		cout << count-1 << " changes in cycle " << cycle << endl;
			count = 0;
			changes = 0;
			cycle++;
			change = true;
			if (white_side != -1) {
				//centres[white_side][1] = 20;
		//		cout << "White side after cycle " << cycle-1 << " = " << white_side << endl;
			}
			for (int i = 0; i < 6; i++) {
				Mat hsv(Size(1,1), CV_8UC3, centres[i]);
				Mat bgr;
				cvtColor(hsv, bgr, CV_HSV2BGR);
				Scalar bgr_colour = bgr.at<Vec3b>(0,0);
				for (int j = 0; j < 9; j++) {
		//			cout << cube_indices[i][j].first << " " << cube_indices[i][j].second << " " << values[i][j] << endl;
					//colourSticker(cube_indices[i][j].first, cube_indices[i][j].second, bgr_colour, image);
				//	cout << cube_indices[i][j].first << " " << cube_indices[i][j].second << endl;
				}
			///	cout << centres[i] << endl;
			}
			for (int i = 0; i < 6; i++) {
		//		cout << starting_centres[i] << " ";
			}
		///	cout << endl;
		}
	}
	Scalar centre_colours[6];
	for (int i = 0; i < 6; i++) {
		centre_colours[i] = centres[i];
	}
	for (int i = 0; i < 6; i++) {
		Mat hsv(Size(1,1), CV_8UC3, centre_colours[i]);
		Mat bgr;
		cvtColor(hsv, bgr, CV_HSV2BGR);
		Vec3b pixel = bgr.at<Vec3b>(0,0);
		centres[starting_centres[i]] = Scalar(pixel[0], pixel[1], pixel[2]);
	}
	for (int i = 0; i < 6; i++) {
		int value = starting_centres[i];
		for (int j = 0; j < 9; j++) {
			positions[cube_indices[i][j].first][cube_indices[i][j].second] = value;
		}
	}
}

void ColourGrouper::updateDifferences(Scalar& centre, Scalar values[9], double differences[10]) {
	double total = 0;
	for (int i = 0; i < 9; i++) {
		Scalar current = values[i];
		double hue = min(abs(current[0]-centre[0]), abs(abs(current[0]-centre[0])-180));
		differences[i] = hue;
		total += hue;
	}
	differences[9] = total;
}

void ColourGrouper::updateCentre(Scalar& centre, Scalar colours[9]) {
	Vec2d total_vec2d;
	Scalar total(0,0,0);
	for (int i = 0; i < 9; i++) {
		total_vec2d += circular.getUnity(colours[i][0]);
		total += colours[i]/9;
	}
	double angle = atan(abs(total_vec2d[1])/abs(total_vec2d[0]));
	total[0] = angle*90/CV_PI;
	if (total_vec2d[0] < 0 && total_vec2d[1] > 0) {
		total[0] = 90 - total[0];
	} else if (total_vec2d[0] < 0 && total_vec2d[1] < 0) {
		total[0] += 90;
	} else if (total_vec2d[0] > 0 && total_vec2d[1] < 0) {
		total[0] = 180 - total[0];
	}
//	cout << total[0] << " " << total[1] << " " << total[2] << endl;
	centre = total;
}

// Methods for Euclidean space

void ColourGrouper::updateDifferences2(Scalar& centre, Scalar values[9], double differences[10]) {
	double total = 0;
	for (int i = 0; i < 9; i++) {
		Scalar current = values[i];
		double difference = sqrt((current[0]-centre[0])*(current[0]-centre[0]) + (current[1]-centre[1])*(current[1]-centre[1]) + (current[2]-centre[2])*(current[2]-centre[2]));
		total += difference;
	}
	differences[9] = total;
}

void ColourGrouper::updateCentre2(Scalar& centre, Scalar colours[9]) {
	Scalar total(0,0,0);
	for (int i = 0; i < 9; i++) {
		total += colours[i];
	}
	total /= 9;
	centre = total;
}

void ColourGrouper::groupColours(Mat& image, Scalar values[][9], int positions[6][9], Scalar centres[6], bool is_hsv) {
	vector<int> random;
	for (int i = 0; i < 54; i++) {
		if (i % 9 != 4) {
			random.push_back(i);
		}
	}
	srand(GetTickCount());
	random_shuffle(random.begin(), random.end());
	Scalar temp_values[6][9];
	pair<int, int> cube_indices[6][9];
	pair<int, int> temp_cube_indices[6][9];
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 9; j++) {
			temp_values[i][j] = values[i][j];
			temp_cube_indices[i][j] = make_pair(i,j);
		}
	}
	int random_index = 0;
	for (int i = 0; i < 48; i++) {
		if (random_index%9 == 4) {
			random_index++;
		}
		int r = random[i];
		values[random_index/9][random_index%9] = temp_values[r/9][r%9];
		cube_indices[random_index/9][random_index%9] = temp_cube_indices[r/9][r%9];
		random_index++;
	}
	if (is_hsv) {
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 9; j++) {
				double h = values[i][j][0];
				double s = values[i][j][1];
				double v = values[i][j][2];
				double correction = 100.0*100.0/255/255;
				values[i][j][0] = correction*s*v*cos(h*CV_PI/90);
				values[i][j][1] = correction*s*v*sin(h*CV_PI/90);
				values[i][j][2] = v*100.0/255;
			}
		}
	}
	int starting_centres[6];
	for (int i = 0; i < 6; i++) {
		starting_centres[i] = i;
		updateCentre2(centres[i], values[i]);
	}
	double differences[6][10];
	for (int i = 0; i < 6; i++) {
		Scalar centre = centres[i];
		double total_difference = 0;
		cube_indices[i][4] = make_pair(i, 4);
		for (int j = 0; j < 9; j++) {
			Scalar current = values[i][j];
			differences[i][j] = sqrt((current[0]-centre[0])*(current[0]-centre[0]) + (current[1]-centre[1])*(current[1]-centre[1]) + (current[2]-centre[2])*(current[2]-centre[2]));
			total_difference += differences[i][j];
		}
		differences[i][9] = total_difference;
	}
	bool change = false;
	int count = 0;
	int cycle = 0;
	int changes = 0;
	while (cycle < 1) {
		change = false;
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 9; j++) {
				for (int k = 0; k < 6; k++) {
					if (i == k) {
						continue;
					}
					for (int l = 0; l < 9; l++) {
						if ((j == 4 && l == 4) || (j != 4 && l != 4)) {
							// Switch the colours
							Scalar temp = values[i][j];
							values[i][j] = values[k][l];
							values[k][l] = temp;
							Scalar colour_A;
							Scalar colour_B;
							updateCentre2(colour_A, values[i]);
							updateCentre2(colour_B, values[k]);
							double differences_A[10];
							double differences_B[10];
							updateDifferences2(colour_A, values[i], differences_A);
							updateDifferences2(colour_B, values[k], differences_B);
							//	cout << differences_A[9] << " " << differences_B[9] << " " << differences[i][9] << " " << differences[k][9] << endl;
							if (differences_A[9] + differences_B[9] < differences[i][9] + differences[k][9]) {
								changes++;
								for (int a = 0; a < 10; a++) {
									differences[i][a] = differences_A[a];
									differences[k][a] = differences_B[a];
								}
								pair<int,int> cube_index = cube_indices[i][j];
								cube_indices[i][j] = cube_indices[k][l];
								cube_indices[k][l] = cube_index;
								centres[i] = colour_A;
								centres[k] = colour_B;
								change = true;
								// cout << difference_A << " " << difference_B << " " << differences[i][j] << " " << differences[k][l] << endl;
								if (j == 4) {
									int starting_index = starting_centres[i];
									starting_centres[i] = starting_centres[k];
									starting_centres[k] = starting_index;
								}
								//cout << i << " " << j << " " << k << " " << l << endl;
								// To break out of the 4 loops:
								i = 6;
								j = 9;
								k = 6;
								l = 9;
								break;
							} else {
								// Switch the colours back
								Scalar temp = values[i][j];
								values[i][j] = values[k][l];
								values[k][l] = temp;
							}
						}
					}
				}
			}
		}
		if (changes > 500) {
			change = false;
		}
		if (!change) {
			count = 0;
			changes = 0;
			cycle++;
			change = true;
		//	cout << endl;
		}
	}
	Scalar centre_colours[6];
	for (int i = 0; i < 6; i++) {
		centre_colours[i] = centres[i];
	}
	for (int i = 0; i < 6; i++) {
		centres[starting_centres[i]] = centre_colours[i];
	}
	for (int i = 0; i < 6; i++) {
		int value = starting_centres[i];
		for (int j = 0; j < 9; j++) {
			positions[cube_indices[i][j].first][cube_indices[i][j].second] = value;
		}
	}
	
	if (is_hsv) {
		for (int i = 0; i < 6; i++) {
			double x = centres[i][0];
			double y = centres[i][1];
			double z = centres[i][2];
			centres[i][0] = 90.0*atan(y/x)/CV_PI;
			if (x < 0) {
				centres[i][0] += 90;
			}
			centres[i][1] = 255.0/100*sqrt(x*x + y*y)/z;
			centres[i][2] = 255.0/100*z;
		}
	}
}