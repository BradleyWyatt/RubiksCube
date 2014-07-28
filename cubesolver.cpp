#include "stdafx.h"
#include <cmath>
#include <iostream>
#include <tuple>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "cubedetector.h"
#include "cubedrawer.h"
#include "cubesolver.h"

using namespace cv;
using namespace std;

CubeSolver::CubeSolver(Cube* cube) {
	this->cube = cube;
}

void CubeSolver::calcCorrectnessScore(int correctness_score[6], int rotations[6]) {
	for (int i = 0; i < 6; i++) {
		int side_piece_neighbours[4];
		for (int j = 0; j < 4; j++) {
			side_piece_neighbours[j] = -1;
		}
		int colour = cube->getColour(i);
		vector<tuple<int,int,int>> locations = cube->getSidePieces(i);
		for (int j = 0; j < 4; j++) {
			tuple<int,int,int> location = locations[j];
			if (cube->getColour(i, location) == colour) {
				vector<int> sides = cube->getSides(location);
				for (int side : sides) {
					if (side != i) {
						side_piece_neighbours[j] = cube->getColour(side, location);
					}
				}
			}
		}
		int num_rotations;
		correctness_score[i] = numSideMatches(i, side_piece_neighbours, num_rotations);
		correctness_score[i] *= 5;
		correctness_score[i] += numCornerMatches(i);
		rotations[i] = num_rotations;
		//	cout << "The score for side " << i << " is " << correctness_score[i] << endl;
	}
}

int CubeSolver::numCornerMatches(int side) {
	int result = 0;
	int colour = cube->getColour(side);
	for (int i = 0; i < 3; i+=2) {
		for (int j = 0; j < 3; j+=2) {
			for (int k = 0; k < 3; k+=2) {
				if (cube->getColour(i, j, k, side) == colour) {
					result++;
				}
			}
		}
	}
	return result;
}

int CubeSolver::numSideMatches(int side, int side_piece_neighbours[4], int& num_rotations) {
	vector<int> sides = cube->getAdjacentSides(side);
	int max = 0;
	for (int i = 0; i < 4; i++) {
		int current = 0;
		for (int j = 0; j < 4; j++) {
			if (side_piece_neighbours[j] == cube->getColour(sides[(i+j)%4])) {
				current++;
			}
		}
		if (current > max) {
			max = current;
			num_rotations = i;
		}
	}
	return max;
}

int CubeSolver::findBestScore(int correctness_score[6], int& score) {
	int index = 0;
	for (int i = 0; i < 6; i++) {
		if (correctness_score[i] > score) {
			score = correctness_score[i];
			index = i;
		}
	}
	return index;
}

int CubeSolver::findSide(int colour) {
	for (int i = 0; i < 6; i++) {
		if (cube->getColour(i) == colour) {
			return i;
		}
	}
	return -1;
}

int CubeSolver::getExtraRotations(int side) {
	if (side == 0) {
		return 0;
	} else if (side == 1) {
		return 2;
	} else if (side == 2) {
		return 3;
	} else {
		return 1;
	}
}

void CubeSolver::findLeftRight(int colour, tuple<int,int,int> locations[4], bool left) {
	int layer = left ? 0 : 2;
	for (int i = 0; i < 4; i++) {
		locations[i] = make_tuple(-1, -1, -1);
	}
	if (cube->getColour(layer, 2, 1, 2) == colour) {
		locations[0] = make_tuple(layer, 2, 1);
	}
	if (cube->getColour(layer, 1, 2, 4) == colour) {
		locations[1] = make_tuple(layer, 1, 2);
	}
	if (cube->getColour(layer, 0, 1, 3) == colour) {
		locations[2] = make_tuple(layer, 0, 1);
	}
	if (cube->getColour(layer, 1, 0, 5) == colour) {
		locations[3] = make_tuple(layer, 1, 0);
	}
}

void CubeSolver::moveLeftRightIntoPlace(tuple<int,int,int> locations[4], int correctness_score[6], int rotations[6], bool left) {
	findLeftRight(cube->getColour(4), locations, left);
	int face = left ? 1 : 0;
	int depth = 2*face;
	for (int i = 0; i < 4; i++) {
		if (get<0>(locations[i]) != -1) {
			int colour = cube->getColour(get<0>(locations[i]), get<1>(locations[i]), get<2>(locations[i]), face);
			int side = findSide(colour);
			int extra_rotations = getExtraRotations(side);
			int top_rotations = left ? 2+rotations[4]+extra_rotations : rotations[4]+extra_rotations;
			top_rotations %= 4;
			if (i == 1) {
				if (top_rotations == 0) {
					continue;
				}
				cube->rotateLayer(0, depth, 1);
			}
			if (top_rotations > 0) {
				cube->rotateLayer(4, 0, top_rotations);
			}
			if (i == 0) {
				cube->rotateLayer(0, depth, 1);
			} else if (i == 1 || i == 2) {
				cube->rotateLayer(0, depth, 3);
			} else {
				cube->rotateLayer(0, depth, 2);
			}
			calcCorrectnessScore(correctness_score, rotations);
			findLeftRight(cube->getColour(4), locations, left);
		}
	}
}

bool CubeSolver::checkCorner(int x, int y, int z, int colour) {
	if (z == 2) {
		for (int i = 0; i < 6; i++) {
			if (cube->getColour(x, y, z, i) == colour) {
				return true;
			}
		}
		return false;
	} else {
		vector<int> sides = cube->getSides(make_tuple(x,y,z));
		bool wrong = false;
		bool is_colour = false;
		for (int side : sides) {
			if (cube->getColour(x, y, z, side) == colour) {
				is_colour = true;
			}
			//	cout << "Colour of side " << side << " = " << getColour(side) << ". Colour of " << x << "  " << y << " " << z << " side " << side << " = " << cube[x][y][z][side] << endl;
			if (cube->getColour(side) != cube->getColour(x, y, z, side)) {
				wrong = true;
			}
		}
		if (colour == -1) {
			return !wrong;
		}
		if (is_colour && wrong) {
			return true;
		} else {
			return false;
		}
	}
}

bool CubeSolver::checkEdge(int x, int y, int z, int colour) {
	if (z == 2) {
		for (int i = 0; i < 6; i++) {
			if (cube->getColour(x, y, z, i) == colour) {
				return false;
			}
		}
		return true;
	} else {
		vector<int> sides = cube->getSides(make_tuple(x,y,z));
		if ((cube->getColour(x, y, z, sides[0]) != colour && cube->getColour(x, y, z, sides[1]) != colour) &&
			(cube->getColour(x, y, z, sides[0]) != cube->getColour(sides[0]) || cube->getColour(x, y, z, sides[1]) != cube->getColour(sides[1]))) {
				return true;
		}
		return false;
	}
}

tuple<int,int,int> CubeSolver::findCorner(int colour) {
	int x = 0;
	int y = 0;
	int z = 2;
	for (int i = 0; i < 8; i++) {
		if (i == 4) {
			z = 0;
		}
		if (checkCorner(x,y,z,colour)) {
			return tuple<int,int,int>(x,y,z);
		}
		int temp_y = y;
		if (x == 2) {
			y = 0;
		} else {
			y = 2;
		}
		x = temp_y;
	}
	return tuple<int,int,int>(-1,-1,-1);
}

tuple<int,int,int> CubeSolver::findEdge(int colour) {
	if (checkEdge(1,0,2,colour)) {
		return tuple<int,int,int>(1,0,2);
	} else if (checkEdge(2,1,2,colour)) {
		return tuple<int,int,int>(2,1,2);
	} else if (checkEdge(1,2,2,colour)) {
		return tuple<int,int,int>(1,2,2);
	} else if (checkEdge(0,1,2,colour)) {
		return tuple<int,int,int>(0,1,2);
	} else if (checkEdge(0,0,1,colour)) {
		return tuple<int,int,int>(0,0,1);
	} else if (checkEdge(2,0,1,colour)) {
		return tuple<int,int,int>(2,0,1);
	} else if (checkEdge(2,2,1,colour)) {
		return tuple<int,int,int>(2,2,1);
	} else if (checkEdge(0,2,1,colour)) {
		return tuple<int,int,int>(0,2,1);
	} else {
		return tuple<int,int,int>(-1,-1,-1);
	}
}

tuple<int,int,int> CubeSolver::findCorrectPosition(tuple<int,int,int> corner) {
	vector<int> sides = cube->getSides(corner);
	vector<int> colour_sides;
	//	cout << "Sides available:" << endl;
	//	for (int side : sides) {
	//		cout << side << endl;
	//	}
	for (int side : sides) {
		int colour = findSide(cube->getColour(side, corner));
		//		cout << side << " -> Colour " << colour << endl;
		//		cout << "Find side = " << findSide(getColour(side, corner)) << endl;
		if (colour != 5) {
			colour_sides.push_back(colour);
		}
	}
	if (colour_sides.size() < 2) {
		return make_tuple(-1,-1,-1);
	}
	int a = colour_sides[0];
	int b = colour_sides[1];
	if (a > b) {
		int temp = a;
		a = b;
		b = temp;
	}
	//	cout << a << " " << b << endl;
	if (a == 0 && b == 2) {
		return make_tuple(2,2,0);
	} else if (a == 1 && b == 2) {
		return make_tuple(0,2,0);
	} else if (a == 1 && b == 3) {
		return make_tuple(0,0,0);
	} else if (a == 0 && b == 3) {
		return make_tuple(2,0,0);
	} else {
		return make_tuple(-1,-1,-1);
	}
}

int CubeSolver::getIndex(int a, int b) {
	if (a == 2 && b == 2) {
		return 0;
	} else if (a == 2 && b == 0) {
		return 1;
	} else if (a == 0 && b == 0) {
		return 2;
	} else {
		assert(a == 0 && b == 2);
		return 3;
	}
}

int CubeSolver::rotationsToFront(tuple<int,int,int> edge) {
	int x = get<0>(edge);
	int y = get<1>(edge);
	if (x == 1 && y == 2) {
		return 0;
	} else if (x == 2 && y == 1) {
		return 1;
	} else if (x == 1 && y == 0) {
		return 2;
	} else if (x == 0 && y == 1) {
		return 3;
	} else {
		return -1;
	}
}

void CubeSolver::uldrAlgorithm1(int number) {
	while (number != 0 && !checkCorner(2,2,0)) {
		cube->rotateLayer(0,0,1);
		cube->rotateLayer(4,0,1);
		cube->rotateLayer(0,0,3);
		cube->rotateLayer(4,0,3);
		number--;
	}
}

void CubeSolver::uldrAlgorithmFinal() {
	while (cube->getColour(2, 2, 0, 0) != cube->getColour(2, 1, 0, 0) || cube->getColour(2, 2, 0, 2) != cube->getColour(1, 2, 0, 2)) {
		uldrAlgorithm();
		if (cube->getDirectionsSize() > move_limit) {
			return;
		}
	}
}

void CubeSolver::uldrAlgorithm() {
	cube->rotateLayer(0,0,1);
	cube->rotateLayer(4,0,1);
	cube->rotateLayer(0,0,3);
	cube->rotateLayer(4,0,3);
}

void CubeSolver::uldrAlgorithmR() {
	cube->rotateLayer(4,0,1);
	cube->rotateLayer(0,0,1);
	cube->rotateLayer(4,0,3);
	cube->rotateLayer(0,0,3);
}

void CubeSolver::layerTwoAlgorithm(bool left) {
	int dir = left ? 3 : 1;
	int layer = left ? 2 : 0;
	cube->rotateLayer(4, 0, dir);
	cube->rotateLayer(0, layer, 1);
	cube->rotateLayer(4, 0, dir+2);
	cube->rotateLayer(0, layer, 3);
	cube->rotateLayer(4, 0, dir+2);
	cube->rotateLayer(2, 0, dir+2);
	cube->rotateLayer(4, 0, dir);
	cube->rotateLayer(2, 0, dir);
}

void CubeSolver::solveLayerOneCross() {
	int correctness_score[6];
	int rotations[6];
	calcCorrectnessScore(correctness_score, rotations);
	int score;
	int index = findBestScore(correctness_score, score);
	//cout << "Starting with side " << index << " which has score " << score << endl;
	if (score < 20) {
		cube->putOnTop(index);
		calcCorrectnessScore(correctness_score, rotations);
		while (correctness_score[4] < 20) {
			tuple<int,int,int> locations[4];
			moveLeftRightIntoPlace(locations, correctness_score, rotations, 0);
			moveLeftRightIntoPlace(locations, correctness_score, rotations, 1);
			if (cube->getColour(1, 2, 2, 2) == cube->getColour(4)) {
				cube->rotateLayer(2, 0, 3);
			} else if (cube->getColour(1, 2, 0, 2) == cube->getColour(4)) {
				while (cube->getColour(1, 2, 2, 4) == cube->getColour(4)) {
					cube->rotateLayer(4, 0, 1);
					if (cube->getDirectionsSize() > move_limit) {
						return;
					}
				}
				cube->rotateLayer(2, 0, 1);
			}
			findLeftRight(cube->getColour(4), locations, 0);
			moveLeftRightIntoPlace(locations, correctness_score, rotations, 0);
			findLeftRight(cube->getColour(4), locations, 1);
			moveLeftRightIntoPlace(locations, correctness_score, rotations, 1);
			if (correctness_score[4] < 20) {
				cube->rotateCube(4, 1);
			}
			if (cube->getDirectionsSize() > move_limit) {
				return;
			}
		}
		cube->rotateLayer(4, 0, rotations[4]);
		cube->putOnTop(5);
	} else {
		cube->rotateLayer(index, 0, rotations[index]);
		int top = (index % 2 == 0) ? index+1 : index-1;
		cube->putOnTop(top);
	}
}

void CubeSolver::solveLayerOneCorners() {
	int count = 0;
	for (tuple<int,int,int> corner = findCorner(cube->getColour(5)); get<0>(corner) != -1; corner = findCorner(cube->getColour(5))) {
		count++;
		int z = get<2>(corner);
		tuple<int,int,int> position = findCorrectPosition(corner);
		//	cout << "Starting position = " << get<0>(corner) << " " << get<1>(corner) << " " << get<2>(corner) << " " << endl;
		//	cout << "Correct position = " << get<0>(position) << " " << get<1>(position) << " " << get<2>(position) << " " << endl;
	/*	cout << "New" << endl;
		cout << get<0>(corner) << " " << get<1>(corner) << " " << get<2>(corner) << endl;
		cout << get<0>(position) << " " << get<1>(position) << " " << get<2>(position) << endl;
		for (int i = 0; i < 6; i++) {
			cout << cube->getColour(get<0>(corner), get<1>(corner), get<2>(corner), i) << " ";
		}
		cout << endl;
		for (int i = 0; i < 6; i++) {
			cout << cube->getColour(get<0>(position), get<1>(position), get<2>(position), i) << " ";
		}*/
		int num_rotations = getIndex(get<0>(position), get<1>(position));
		cube->rotateCube(4, num_rotations);
		///cout << num_rotations << endl;
		if (z == 2) {
			num_rotations = getIndex(get<0>(corner), get<1>(corner))-num_rotations+4;
			///cout << num_rotations << endl;
			cube->rotateLayer(4,0, num_rotations);
			uldrAlgorithm1();
		} else {
			uldrAlgorithm1(1);
		}
		if (cube->getDirectionsSize() > move_limit || count > move_limit) {
			return;
		}
	}
}

void CubeSolver::solveLayerTwo() {
	for (tuple<int,int,int> edge = findEdge(cube->getColour(4)); get<0>(edge) != -1; edge = findEdge(cube->getColour(4))) {
		if (get<2>(edge) == 2) {
			vector<int> sides = cube->getSides(edge);
			int side;
			for (int s : sides) {
				if (s != 4) {
					side = s;
				}
			}
			int colour_top = cube->getColour(4, edge);
			int colour_side = cube->getColour(side, edge);
			//	cout << get<0>(edge) << " " << get<1>(edge) << " " << get<2>(edge) << endl;
			//	cout << "side = " << side << ". Colour_top = " << colour_top << ". Colour_side = " << colour_side << endl;
			//	cout << "findSide(colour_top) = " << findSide(colour_top) << ". findSide(colour_side) = " << findSide(colour_side) << endl;
			//	cout << getExtraRotations(side) << " " << getExtraRotations(findSide(colour_side)) << endl;
			cube->rotateLayer(4, 0, getExtraRotations(side) - getExtraRotations(findSide(colour_side)) + 4);
			cube->rotateCube(4, getExtraRotations(findSide(colour_side)) + 1);
			if (findSide(cube->getColour(1, 2, 2, 4)) == 0) {
				layerTwoAlgorithm(0);
			} else {
				layerTwoAlgorithm(1);
			}
		} else {
			//	cout << get<0>(edge) << " " << get<1>(edge) << " " << get<2>(edge) << endl;
			if (get<1>(edge) != 2) {
				cube->rotateCube(4, 2);
				get<0>(edge) = 2 - get<0>(edge);
			}
			if (get<0>(edge) == 2) {
				layerTwoAlgorithm(0);
			} else {
				layerTwoAlgorithm(1);
			}
		}
		if (cube->getDirectionsSize() > move_limit) {
			return;
		}
	}
}

void CubeSolver::getLayerThreeCross() {
	int side = cube->getColour(4);
	bool a = (cube->getColour(1, 0, 2, 4) == side);
	bool b = (cube->getColour(2, 1, 2, 4) == side);
	bool c = (cube->getColour(1, 2, 2, 4) == side);
	bool d = (cube->getColour(0, 1, 2, 4) == side);
	if (a && b && c && d) {
		return;
	}
	if (!a && !b && !c && !d) {
		cube->rotateLayer(2, 0, 1);
		uldrAlgorithm();
		cube->rotateLayer(2, 0, 3);
		a = (cube->getColour(1, 0, 2, 4) == side);
		b = (cube->getColour(2, 1, 2, 4) == side);
		c = (cube->getColour(1, 2, 2, 4) == side);
		d = (cube->getColour(0, 1, 2, 4) == side);
	}
	if (a && b) {
		cube->rotateLayer(4, 0, 3);
		cube->rotateLayer(2, 0, 1);
		uldrAlgorithmR();
		cube->rotateLayer(2, 0, 3);
	} else if (b && c) {
		cube->rotateLayer(4, 0, 2);
		cube->rotateLayer(2, 0, 1);
		uldrAlgorithmR();
		cube->rotateLayer(2, 0, 3);
	} else if (c && d) {
		cube->rotateLayer(4, 0, 1);
		cube->rotateLayer(2, 0, 1);
		uldrAlgorithmR();
		cube->rotateLayer(2, 0, 3);
	} else if (d && a) {
		cube->rotateLayer(2, 0, 1);
		uldrAlgorithmR();
		cube->rotateLayer(2, 0, 3);
	} else if (a && c) {
		cube->rotateLayer(4, 0, 1);
		cube->rotateLayer(2, 0, 1);
		uldrAlgorithm();
		cube->rotateLayer(2, 0, 3);
	} else if (b && d) {
		cube->rotateLayer(2, 0, 1);
		uldrAlgorithm();
		cube->rotateLayer(2, 0, 3);
	}
}

void CubeSolver::moveTopLayerEdges() {
	cube->rotateLayer(0, 0, 1);
	cube->rotateLayer(4, 0, 1);
	cube->rotateLayer(0, 0, 3);
	cube->rotateLayer(4, 0, 1);
	cube->rotateLayer(0, 0, 1);
	cube->rotateLayer(4, 0, 2);
	cube->rotateLayer(0, 0, 3);
}

void CubeSolver::orientateLayerThreeCross() {
	int sides[4] = {0,2,1,3};
	int colour_sides[4];
	int max_matches = 0;
	int index = 0;
	while (max_matches != 4) {
		colour_sides[0] = findSide(cube->getColour(0, make_tuple(2,1,2)));
		colour_sides[1] = findSide(cube->getColour(2, make_tuple(1,2,2)));
		colour_sides[2] = findSide(cube->getColour(1, make_tuple(0,1,2)));
		colour_sides[3] = findSide(cube->getColour(3, make_tuple(1,0,2)));
		max_matches = 0;
		int best_last_match = 0;
		for (int i = 0; i < 4; i++) {
			int num_matches = 0;
			int last_match = -1;
			//		cout << "i = " << i << endl;
			for (int j = 0; j < 4; j++) {
				//			cout << "j = " << j << endl;
				//			cout << "sides[j] = " << sides[j] << " colour_sides[(i+j)%4] = " << colour_sides[(i+j)%4] << endl;
				if (sides[(i+j)%4] == colour_sides[j]) {
					num_matches++;
					if (last_match == -1 || last_match == j-1) {
						last_match = j;
					}
				}
			}
			if (num_matches > max_matches) {
				max_matches = num_matches;
				index = i;
				best_last_match = last_match;
			}
		}
		//	cout << max_matches << " " << index << " " << best_last_match << endl;
		if (max_matches != 4) {
			cube->rotateLayer(4, 0, index);
			cube->rotateCube(4, best_last_match);
			moveTopLayerEdges();
		}
		if (cube->getDirectionsSize() > move_limit) {
			return;
		}
	}
	cube->rotateLayer(4, 0, index);
}

void CubeSolver::solveLayerThreeCross() {
	getLayerThreeCross();
	orientateLayerThreeCross();
}

bool CubeSolver::checkLayerThreeCorner(int x, int y, int z) {
	vector<int> colours;
	vector<int> sides;
	for (int i = 0; i < 6; i++) {
		int colour = cube->getColour(x, y, z, i);
		if (colour != -1) {
			colours.push_back(colour);
			sides.push_back(i);
		}
	}
	for (int colour : colours) {
		bool found = false;
		for (int side : sides) {
			if (cube->getColour(side) == colour) {
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}
	return true;
}

int CubeSolver::checkLayerThreeCorners() {
	int x = 2;
	int y = 2;
	int z = 2;
	int num_correct = 0;
	int found = 0;
	for (int i = 0; i < 4; i++) {
		bool current = checkLayerThreeCorner(x,y,z);
		//cout << i << " " << found << " " << current << endl;
		if (current && found != 0) {
			return -1;
		} else if (current) {
			found = i;
		}
		int temp_y = y;
		if (x == 2) {
			y = 0;
		} else {
			y = 2;
		}
		x = temp_y;
	}
	return found;
}

void CubeSolver::cornerAlgorithm() {
	cube->rotateLayer(4, 0, 1);
	cube->rotateLayer(0, 0, 1);
	cube->rotateLayer(4, 0, 3);
	cube->rotateLayer(0, 2, 1);
	cube->rotateLayer(4, 0, 1);
	cube->rotateLayer(0, 0, 3);
	cube->rotateLayer(4, 0, 3);
	cube->rotateLayer(0, 2, 3);
}

void CubeSolver::solveLayerThreeCorners() {
	for (int correct_corner = checkLayerThreeCorners(); correct_corner != -1; correct_corner = checkLayerThreeCorners()) {
		cube->rotateCube(4, correct_corner);
		cornerAlgorithm();
		if (cube->getDirectionsSize() > move_limit) {
			return;
		}
	}
	int correctness_score[6];
	int rotations[6];
	calcCorrectnessScore(correctness_score, rotations);
	bool complete = true;
	for (int i = 0; i < 6; i++) {
		if (correctness_score[i] < 24) {
			complete = false;
			break;
		}
	}
	if (!complete) {
		cube->putOnTop(5);
		calcCorrectnessScore(correctness_score, rotations);
		for (int i = 0; i < 4 && !complete; i++) {
			uldrAlgorithmFinal();
			cube->rotateLayer(5, 0, 3);
			calcCorrectnessScore(correctness_score, rotations);
			complete = true;
			for (int j = 0; j < 6; j++) {
				if (correctness_score[j] < 24) {
					complete = false;
					break;
				}
			}
		}
	}
	calcCorrectnessScore(correctness_score, rotations);
}

bool CubeSolver::solve() {
	//encoded_directions.clear();
	//cube->reset();
	cube->rotateCube(1, 1, false);
	cube->rotateCube(2, 1, false);
	//cout << "STARTING SOLVE" << endl;
	solveLayerOneCross();
	//cout << cube->getDirectionsSize() << endl;
	solveLayerOneCorners();
	//cout << cube->getDirectionsSize() << endl;
	solveLayerTwo();
	//cout << cube->getDirectionsSize() << endl;
	solveLayerThreeCross();
	//cout << cube->getDirectionsSize() << endl;
	solveLayerThreeCorners();
	//cout << cube->getDirectionsSize() << endl;
	if (cube->getDirectionsSize() > move_limit || !cube->checkSolved()) {
		return false;
	}
	return true;
}

int CubeSolver::cornerValue(int i, int j, int k) {
	int front = cube->getColour(2);
	int back = cube->getColour(3);
	//cout << "Front " << front << " Back " << back << " " << i << " " << j << " " << k << endl;
	for (int l = 0; l < 3; l++) {
		//cout << cube->getColour(i,j,k,2*l) << " " << cube->getColour(i,j,k,2*l + 1)<< endl;
		int colour = max(cube->getColour(i,j,k,2*l), cube->getColour(i,j,k,2*l + 1));
		if (colour == front || colour == back) {
			int val = (i + j + k) % 4;
			//cout << colour << " " << val << " " << l << endl;
			if (l == 0) {
				if (val == 0) {
					return 2;
				} else {
					return 1;
				}
			} else if (l == 2) {
				if (val == 0) {
					return 1;
				} else {
					return 2;
				}
			}
			return 0;
		}
	}
}

int CubeSolver::edgeValue(int i, int j, int k) {
	int front = cube->getColour(2);
	int back = cube->getColour(3);
	int left = cube->getColour(1);
	int right = cube->getColour(0);
	int up = cube->getColour(4);
	int down = cube->getColour(5);
	bool complete = false;
	for (int l = 0; l < 3; l++) {
		int colour = max(cube->getColour(i,j,k,2*l), cube->getColour(i,j,k,2*l + 1));
		if (colour == front || colour == back) {
		//	cout << front << " " << back << " " << i << " " << j << " " << k << " " << l << endl;
			if (l == 1 || l == 2 && j == 1) {
				return 1;
			}
			return 0;
		}
	}
	for (int l = 0; l < 3; l++) {
		int colour = max(cube->getColour(i,j,k,2*l), cube->getColour(i,j,k,2*l + 1));
		if (colour == up || colour == down) {
		//	cout << "colours " << right << " " << left << " " << front << " " << back << " " << up << " " << down << endl;
		//	cout << i << " " << j << " " << k << " " << l << endl;
			colour = max(cube->getColour(i,j,k,0), cube->getColour(i,j,k,1));
			if (l == 1 || (l == 2 && (colour == left || colour == right))) {
				return 1;
			}
		}
	}
	return 0;
}

bool moveIntoPlace(Cube &cube, vector<int> values, int x, int y, int z) {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				if ((i == 1 && j == 1 && k == 1) || (i == x && j == y && k == z)) {
					continue;
				}
				if ((values.size() == 2 && ((i + j + k) % 2 == 1)) || (values.size() == 3 && (i%2 == 0 && j%2 == 0 && k%2 == 0))) {
					vector<int> compare;
					for (int l = 0; l < 6; l++) {
						int colour = cube.getColour(i,j,k,l);
						if (colour != -1) {
							compare.push_back(colour);
						}
					}
					assert(compare.size() == values.size());
					sort(compare.begin(), compare.end());
					if (compare == values) {
			//			cout << "Swapping " << x << " " << y << " " << z << " with " << i << " " << j << " " << k << endl;
						cube.swap(i,j,k,x,y,z);
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool CubeSolver::checkSolvable() {
	int corner = 0;
	int edge = 0;
	int front = cube->getColour(2);
	int back = cube->getColour(3);
	int left = cube->getColour(1);
	int right = cube->getColour(0);
	int up = cube->getColour(4);
	int down = cube->getColour(5);
	//cout << right << " " << left << " " << front << " " << back << " " << up << " " << down << endl;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				if (i == 1 && j == 1 && k == 1) {
					continue;
				}
				if (i%2 == 0 && j%2 == 0 && k%2 == 0) {
					corner += cornerValue(i,j,k);
				} else if ((i + j + k) % 2 == 1) {
					edge += edgeValue(i,j,k);
				}
			}
		}
	}
	int positions[6][9];
	cube->getPositions(positions);
	Cube modified(positions);
	int moves = 0;
	bool complete = false;
	int attempts = 0;
	while (!complete && attempts < 21) {
		//cout << "not complete" << endl;
		complete = true;
		attempts++;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				for (int k = 0; k < 3; k++) {
					if (i == 1 && j == 1 && k == 1) {
						continue;
					}
					if ((i%2 == 0 && j%2 == 0 && k%2 == 0) || ((i + j + k) % 2 == 1)) {
						vector<int> values;
						for (int l = 0; l < 6; l++) {
							int colour = modified.getColour(i,j,k,l);
							if (colour != -1) {
								values.push_back(colour);
							}
						}
						assert(values.size() == 2 || values.size() == 3);
						sort(values.begin(), values.end());
						vector<int> sides;
						if (i != 1) {
							sides.push_back(modified.getColour(1-(i/2)));
						}
						if (j != 1) {
							sides.push_back(modified.getColour(3-(j/2)));
						}
						if (k != 1) {
							sides.push_back(modified.getColour(5-(k/2)));
						}
						sort(sides.begin(), sides.end());
						if (!equal(sides.begin(), sides.end(), values.begin())) {
							/*cout << "Piece needs moving: " << i << " " << j << " " << k << endl;
							for (int a : values) {
								cout << a << " ";
							}
							cout << endl;
							for (int a : sides) {
								cout << a << " ";
							}
							cout << endl;*/
							if (!moveIntoPlace(modified,sides,i,j,k)) {
							//	cout << "Illegal cube" << endl;
								return false;
							}
							moves++;
							complete = false;
						}
					}
				}
			}
		}
	}
	//cout << right << " " << left << " " << front << " " << back << " " << up << " " << down << endl;
	//cout << moves << " " << corner << " " << edge << endl;
	if (!complete || moves % 2 != 0 || corner % 3 != 0 || edge % 2 != 0) {
		return false;
	}
	return true;
}