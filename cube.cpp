#include "stdafx.h"
#include <cmath>
#include <iostream>
#include <tuple>

#include "cube.h"
#include "cubedrawernet.h"
#include "cubedraweruser.h"
#include "position.h"
#include "stringnames.h"

#include <opencv2/core/core.hpp>

using namespace cv;
using namespace std;

Cube::Cube() {
	reset();
}

Cube::Cube(int positions[6][9]) {
	reset();
	loadPositions(positions);
}

Cube::Cube(int positions[6][9], Scalar* colours) {
	reset();
	loadPositions(positions);
	setColours(colours);
}

void Cube::setDetector(CubeDetector* cube_detector) {
	this->cube_detector = cube_detector;
}

int Cube::getDirectionsSize() {
	return encoded_directions.size();
}

int Cube::getColour(int x, int y, int z, int dir) {
	return cube[x][y][z][dir];
}

void Cube::setColours(Scalar input[6]) {
	for (int i = 0; i < 6; i++) {
		colours[i] = input[i];
	}
}

void Cube::getColours(Scalar output[6]) {
	for (int i = 0; i < 6; i++) {
		output[i] = colours[i];
	}
}

void Cube::reset() {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				for (int l = 0; l < 6; l++) {
					cube[i][j][k][l] = -1;
				}
			}
		}
	}
}

void Cube::loadPositions(int stickers[6][9]) {
	for (int i = 0; i < 9; i++) {
		cube[i%3][0][2-(i/3)][3] = stickers[0][i];
		cube[2][i%3][2-(i/3)][0] = stickers[1][i];
		cube[2-(i%3)][2][2-(i/3)][2] = stickers[2][i];
		cube[0][2-(i%3)][2-(i/3)][1] = stickers[3][i];
		cube[2-(i/3)][2-(i%3)][2][4] = stickers[4][i];
		cube[i/3][2-(i%3)][0][5] = stickers[5][i];
	}
}

void Cube::getPositions(int stickers[6][9]) {
	for (int i = 0; i < 9; i++) {
		stickers[0][i] = cube[i%3][0][2-(i/3)][3];
		stickers[1][i] = cube[2][i%3][2-(i/3)][0];
		stickers[2][i] = cube[2-(i%3)][2][2-(i/3)][2];
		stickers[3][i] = cube[0][2-(i%3)][2-(i/3)][1];
		stickers[4][i] = cube[2-(i/3)][2-(i%3)][2][4];
		stickers[5][i] = cube[i/3][2-(i%3)][0][5];
	}
}

void Cube::rotate(tuple<int, int, int, int> rotations[4], int num_rotations) {
	for (int i = 0; i < num_rotations; i++) {
		int temp = cube[get<0>(rotations[3])][get<1>(rotations[3])][get<2>(rotations[3])][get<3>(rotations[3])];
		cube[get<0>(rotations[3])][get<1>(rotations[3])][get<2>(rotations[3])][get<3>(rotations[3])] = cube[get<0>(rotations[2])][get<1>(rotations[2])][get<2>(rotations[2])][get<3>(rotations[2])];
		cube[get<0>(rotations[2])][get<1>(rotations[2])][get<2>(rotations[2])][get<3>(rotations[2])] = cube[get<0>(rotations[1])][get<1>(rotations[1])][get<2>(rotations[1])][get<3>(rotations[1])];
		cube[get<0>(rotations[1])][get<1>(rotations[1])][get<2>(rotations[1])][get<3>(rotations[1])] = cube[get<0>(rotations[0])][get<1>(rotations[0])][get<2>(rotations[0])][get<3>(rotations[0])];
		cube[get<0>(rotations[0])][get<1>(rotations[0])][get<2>(rotations[0])][get<3>(rotations[0])] = temp;
	}
}

void Cube::rotateCube(int side, int num_rotations, bool show_messages) {
	for (int i = 0; i < 3; i++) {
		rotateLayer(side, i, num_rotations, false);
	}
	if (show_messages && num_rotations % 4 != 0) {
		makeStringRotateCube(side, num_rotations);
	}
}

void Cube::rotateLayer(int side, int depth, int num_rotations, bool show_messages) {
	int layer = (side%2 == 0) ? 2-depth : depth;
	int dir = (side%2 == 0) ? 1 : -1;
	int sticker_side = depth == 0 ? side : side+dir;
	int reverse = (side%2 == 1) ? 3 : 1;
	num_rotations %= 4;
	int num_times = num_rotations*reverse;
	tuple<int,int,int,int> rotations[4];
	if (side == 0 || side == 1) {
		if (depth == 0 || depth == 2) {
			rotations[0] = make_tuple(layer, 0, 2, sticker_side);
			rotations[1] = make_tuple(layer, 0, 0, sticker_side);
			rotations[2] = make_tuple(layer, 2, 0, sticker_side);
			rotations[3] = make_tuple(layer, 2, 2, sticker_side);
			rotate(rotations, num_rotations*reverse);

			rotations[0] = make_tuple(layer, 0, 1, sticker_side);
			rotations[1] = make_tuple(layer, 1, 0, sticker_side);
			rotations[2] = make_tuple(layer, 2, 1, sticker_side);
			rotations[3] = make_tuple(layer, 1, 2, sticker_side);
			rotate(rotations, num_rotations*reverse);
		}
		for (int j = 0; j < 3; j++) {
			rotations[0] = make_tuple(layer, 0, j, 3);
			rotations[1] = make_tuple(layer, 2-j, 0, 5);
			rotations[2] = make_tuple(layer, 2, 2-j, 2);
			rotations[3] = make_tuple(layer, j, 2, 4);
			rotate(rotations, num_rotations*reverse);
		}
	} else if (side == 2 || side == 3) {
		if (depth == 0 || depth == 2) {
			rotations[0] = make_tuple(2, layer, 0, sticker_side);
			rotations[1] = make_tuple(0, layer, 0, sticker_side);
			rotations[2] = make_tuple(0, layer, 2, sticker_side);
			rotations[3] = make_tuple(2, layer, 2, sticker_side);
			rotate(rotations, num_rotations*reverse);

			rotations[0] = make_tuple(1, layer, 0, sticker_side);
			rotations[1] = make_tuple(0, layer, 1, sticker_side);
			rotations[2] = make_tuple(1, layer, 2, sticker_side);
			rotations[3] = make_tuple(2, layer, 1, sticker_side);
			rotate(rotations, num_rotations*reverse);
		}
		for (int j = 0; j < 3; j++) {
			rotations[0] = make_tuple(j, layer, 0, 5);
			rotations[1] = make_tuple(0, layer, 2-j, 1);
			rotations[2] = make_tuple(2-j, layer, 2, 4);
			rotations[3] = make_tuple(2, layer, j, 0);
			rotate(rotations, num_rotations*reverse);
		}
	} else if (side == 4 || side == 5) {
		if (depth == 0 || depth == 2) {
			rotations[0] = make_tuple(0, 2, layer, sticker_side);
			rotations[1] = make_tuple(0, 0, layer, sticker_side);
			rotations[2] = make_tuple(2, 0, layer, sticker_side);
			rotations[3] = make_tuple(2, 2, layer, sticker_side);
			rotate(rotations, num_rotations*reverse);

			rotations[0] = make_tuple(0, 1, layer, sticker_side);
			rotations[1] = make_tuple(1, 0, layer, sticker_side);
			rotations[2] = make_tuple(2, 1, layer, sticker_side);
			rotations[3] = make_tuple(1, 2, layer, sticker_side);
			rotate(rotations, num_rotations*reverse);
		}
		for (int j = 0; j < 3; j++) {
			rotations[0] = make_tuple(0, j, layer, 1);
			rotations[1] = make_tuple(2-j, 0, layer, 3);
			rotations[2] = make_tuple(2, 2-j, layer, 0);
			rotations[3] = make_tuple(j, 2, layer, 2);
			rotate(rotations, num_rotations*reverse);
		}
	}
	if (show_messages) {
		if (num_rotations != 0) {
			string directions = makeStringRotateLayer(side, depth, num_rotations);
		}
	}
}

void Cube::putOnTop(int side) {
	int rotate_side = -1;
	int num_rotations = 1;
	if (side == 0) {
		rotate_side = 3;
	} else if (side == 1) {
		rotate_side = 2;
	} else if (side == 2) {
		rotate_side = 0;
	} else if (side == 3) {
		rotate_side = 1;
	} else if (side == 4) {
		return;
	} else if (side == 5) {
		rotate_side = 0;
		num_rotations = 2;
	}
	string directions = makeStringRotateCube(rotate_side, num_rotations);
	if (!directions.empty()) {
		for (int i = 0; i < 3; i++) {
			rotateLayer(rotate_side, i, num_rotations, false);
		}
	}
}

int Cube::getColour(int side) {
	if (side == 0) {
		return cube[2][1][1][0];
	} else if (side == 1) {
		return cube[0][1][1][1];
	} else if (side == 2) {
		return cube[1][2][1][2];
	} else if (side == 3) {
		return cube[1][0][1][3];
	} else if (side == 4) {
		return cube[1][1][2][4];
	} else if (side == 5) {
		return cube[1][1][0][5];
	}
	return -1;
}

int Cube::getColour(int side, tuple<int,int,int> location) {
	int x = get<0>(location);
	int y = get<1>(location);
	int z = get<2>(location);
	return cube[x][y][z][side];
}

tuple<int,int,int> Cube::getCentre(int side) {
	if (side == 0) {
		return make_tuple(2, 1, 1);
	} else if (side == 1) {
		return make_tuple(0, 1, 1);
	} else if (side == 2) {
		return make_tuple(1, 2, 1);
	} else if (side == 3) {
		return make_tuple(1, 0, 1);
	} else if (side == 4) {
		return make_tuple(1, 1, 2);
	} else if (side == 5) {
		return make_tuple(1, 1, 0);
	}
}

string Cube::makeStringRotateLayer(int side, int layer, int num_times) {
	num_times %= 4;
	if (num_times == 0) {
		return "";
	}
	string directions = "L";
	if (layer == 2) {
		num_times = 4 - num_times;
		if (side % 2 == 0) {
			side++;
		} else {
			side--;
		}
	}
	directions += to_string(side);
	directions += to_string(num_times);
	encoded_directions.push_back(directions);
	return directions;
}

string Cube::makeStringRotateCube(int rotate_side, int num_rotations) {
	string direction = "R";
	if (rotate_side % 2 == 1) {
		rotate_side--;
		num_rotations = 4 - num_rotations;
	}
	direction += to_string(rotate_side);
	direction += to_string(num_rotations);
	encoded_directions.push_back(direction);
	return direction;
}

vector<tuple<int,int,int>> Cube::getSidePieces(int side) {
	vector<tuple<int,int,int>> side_pieces;
	side_pieces.resize(4);
	int layer = (side%2 == 0) ? 2 : 0;
	if (side == 0 || side == 1) {
		side_pieces[0] = make_tuple(layer,1,2);
		side_pieces[1] = make_tuple(layer,0,1);
		side_pieces[2] = make_tuple(layer,1,0);
		side_pieces[3] = make_tuple(layer,2,1);
	} else if (side == 2 || side == 3) {
		side_pieces[0] = make_tuple(1,layer,2);
		side_pieces[1] = make_tuple(2,layer,1);
		side_pieces[2] = make_tuple(1,layer,0);
		side_pieces[3] = make_tuple(0,layer,1);
	} else if (side == 4 || side == 5) {
		side_pieces[0] = make_tuple(2,1,layer);
		side_pieces[1] = make_tuple(1,2,layer);
		side_pieces[2] = make_tuple(0,1,layer);
		side_pieces[3] = make_tuple(1,0,layer);
	}
	if (side == 1 || side == 3 || side == 5) {
		reverse(side_pieces.begin(), side_pieces.end());
	}
	return side_pieces;
}

vector<int> Cube::getSides(tuple<int,int,int> location) {
	int x = get<0>(location);
	int y = get<1>(location);
	int z = get<2>(location);
	vector<int> sides;
	if (x == 0) {
		sides.push_back(1);
	} else if (x == 2) {
		sides.push_back(0);
	}
	if (y == 0) {
		sides.push_back(3);
	} else if (y == 2) {
		sides.push_back(2);
	}
	if (z == 0) {
		sides.push_back(5);
	} else if (z == 2) {
		sides.push_back(4);
	}
	return sides;
}

vector<int> Cube::getAdjacentSides(int side) {
	vector<int> sides;
	sides.resize(4);
	if (side == 0) {
		sides[0] = 4;
		sides[1] = 3;
		sides[2] = 5;
		sides[3] = 2;
	} else if (side == 1) {
		sides[0] = 2;
		sides[1] = 5;
		sides[2] = 3;
		sides[3] = 4;
	} else if (side == 2) {
		sides[0] = 4;
		sides[1] = 0;
		sides[2] = 5;
		sides[3] = 1;
	} else if (side == 3) {
		sides[0] = 1;
		sides[1] = 5;
		sides[2] = 0;
		sides[3] = 4;
	} else if (side == 4) {
		sides[0] = 0;
		sides[1] = 2;
		sides[2] = 1;
		sides[3] = 3;
	} else if (side == 5) {
		sides[0] = 3;
		sides[1] = 1;
		sides[2] = 2;
		sides[3] = 0;
	}
	return sides;
}

void Cube::simplifyDirections() {
	vector<string> uldr;
	uldr.push_back("L01");
	uldr.push_back("L41");
	uldr.push_back("L03");
	uldr.push_back("L43");
	vector<string> uldrR;
	uldrR.push_back("L41");
	uldrR.push_back("L01");
	uldrR.push_back("L43");
	uldrR.push_back("L03");
	cout << endl << "Length of solution without optimisations: " << encoded_directions.size() << " moves" << endl;
	int size = encoded_directions.size();
	int limit = size-19;
	for (int i = 0; i < limit; i++) {
		int repeats = 0;
		for (int j = 0; j < 20; j++) {
			if (encoded_directions[i+j].compare(uldr[j%4]) != 0) {
				break;
			}
			if ((j+1) % 4 == 0) {
				repeats++;
			}
		}
		if (repeats >= 4) {
			encoded_directions.erase(encoded_directions.begin()+i, encoded_directions.begin()+i+repeats*4);
			for (int j = 0; j < 6-repeats; j++) {
				encoded_directions.insert(encoded_directions.begin()+i, uldrR.begin(), uldrR.end());
			}
			limit -= repeats*4;
		}
	}
	size = encoded_directions.size();
	limit = size - 1;
	for (int i = 0; i < limit; i++) {
		if (encoded_directions[i][0] == encoded_directions[i+1][0] && encoded_directions[i][1] == encoded_directions[i+1][1]) {
			string val1 = encoded_directions[i].substr(2,1);
			string val2 = encoded_directions[i+1].substr(2,1);
			int value = stoi(val1) + stoi(val2);
			string replacement = encoded_directions[i].substr(0,2) + to_string(value);
			encoded_directions.erase(encoded_directions.begin()+i, encoded_directions.begin()+i+2);
			encoded_directions.insert(encoded_directions.begin()+i, replacement);
			i--;
			limit--;
		}
	}
	cout << solution_length << encoded_directions.size() << " moves" << endl << endl;
}

void Cube::decodeDirections() {
	CubeDrawerNet cube_drawer_net(200);
	CubeDrawerUser cube_drawer_user(200);
	Position position;
	position.getPositionFromFile();
	vector<vector<pair<int, int>>> window_positions = position.getPositions();
	int positions[6][9];
	getPositions(positions);
	Mat start_image = cube_drawer_user.colourCube(positions, colours);
	imshow(current_user_view, start_image);
	Size size(200, 200);
	Mat blank = imread("images\\blank.png");
	resize(blank, blank, size);
	imshow(instruction, blank);
	//moveWindow(instruction, 567, 400);
	Mat l_images[6][2][2];
	Mat r_images[6][2];
	Mat choice[2];
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 2; j++) {
			Mat image_clockwise = imread("images\\l" + to_string(i) + "c" + to_string(j) + ".png");
			resize(image_clockwise, image_clockwise, size);
			Mat image_anticlockwise = imread("images\\l" + to_string(i) + "a" + to_string(j) + ".png");
			resize(image_anticlockwise, image_anticlockwise, size);
			Mat cube_rotate = imread("images\\r" + to_string(i) + "" + to_string(j) + ".png");
			resize(cube_rotate, cube_rotate, size);
			l_images[i][0][j] = image_clockwise;
			l_images[i][1][j] = image_anticlockwise;
			r_images[i][j] = cube_rotate;
		}
	}
	for (string encoded : encoded_directions) {
		string decoded;
		int side = stoi(encoded.substr(1,1));
		int rotations = stoi(encoded.substr(2,1));
		rotations %= 4;
		if (rotations == 0) {
			continue;
		}
		string code = encoded.substr(0,1);
		if (code.compare("R") == 0) {
			decoded += rotate_cube_message;
			int dir = 1;
			while (rotations > 2) {
				rotations -= 2;
				side += dir;
				dir *= -1;
			}
			if (side == 0) {
				decoded += side_0_rotate_cube_message;
			} else if (side == 1) {
				decoded += side_1_rotate_cube_message;
			} else if (side == 2) {
				decoded += side_2_rotate_cube_message;
			} else if (side == 3) {
				decoded += side_3_rotate_cube_message;
			} else if (side == 4) {
				decoded += side_4_rotate_cube_message;
			} else if (side == 5) {
				decoded += side_5_rotate_cube_message;
			}
			choice[0] = r_images[side][0].clone();
			choice[1] = r_images[side][1].clone();
			for (int i = 0; i < 9; i++) {
				updated.push_back(i);
			}
		} else {
			assert(code.compare("L") == 0);
			decoded += rotate_message;
			bool clockwise = true;
			if (rotations == 3) {
				clockwise = false;
			}
			if (side == 0) {
				decoded += right_message;
				updated.push_back(2);
				updated.push_back(5);
				updated.push_back(8);
			} else if (side == 1) {
				decoded += left_message;
				updated.push_back(0);
				updated.push_back(3);
				updated.push_back(6);
			} else if (side == 2) {
				decoded += front_message;
			} else if (side == 3) {
				decoded += back_message;
				for (int i = 0; i < 9; i++) {
					if (i == 4) {
						continue;
					}
					updated.push_back(i);
				}
			} else if (side == 4) {
				decoded += top_message;
				updated.push_back(0);
				updated.push_back(1);
				updated.push_back(2);
			} else if (side == 5) {
				decoded += bottom_message;
				updated.push_back(6);
				updated.push_back(7);
				updated.push_back(8);
			}
			decoded += rotate_face_message;
			if (clockwise) {
				choice[0] = l_images[side][0][0].clone();
				choice[1] = l_images[side][0][1].clone();
				decoded += clockwise_message;
			} else {
				choice[0] = l_images[side][1][0].clone();
				choice[1] = l_images[side][1][1].clone();
				decoded += anticlockwise_message;
			}
		}
		Mat instruction_image = choice[0];
		if (rotations == 2) {
			decoded.replace(decoded.find("90"), 2, "180");
			instruction_image = choice[1];
		}
		if (code.compare("R") == 0) {
			rotateCube(side, rotations, false);
		} else {
			rotateLayer(side, 0, rotations, false);
		}
		cout << decoded << endl;
		imshow(instruction, instruction_image);
		imshow(cube_net, cube_drawer_net.colourCube(positions, colours));
		moveWindow(cube_net, window_positions[2][0].first, window_positions[2][0].second);
		getPositions(positions);
		drawCube(cube_drawer_user, window_positions);
	}
	cout << completed_message << endl;
}

void Cube::drawCube(CubeDrawerUser& cube_drawer_user, vector<vector<pair<int, int>>>& window_positions) {
	int positions[6][9];
	getPositions(positions);
	cube_drawer_user.updateCube(positions, colours, window_positions);

	///////////////////////////////////////////////////////
	// Choose ONE of the following lines

	// 1) Advances only when the correct move has been detected
	cube_detector->checkColours(positions, colours, updated);

	// 2) Press any key to assume that the cube has been correctly transformed
	//waitKey(0);
	///////////////////////////////////////////////////////

	updated.clear();
}

void Cube::replayMoves(int positions[6][9], Scalar centre_colours[6]) {
	loadPositions(positions);
	setColours(centre_colours);
	rotateCube(1, 1, false);
	rotateCube(2, 1, false);
	simplifyDirections();
	decodeDirections();
}

Mat Cube::getImage() {
	CubeDrawerUser cube_drawer_user;
	cube_drawer_user.drawCubeOutline(200);
	int positions[6][9];
	getPositions(positions);
	return cube_drawer_user.colourCube(positions, colours);
}

void Cube::swap(int i, int j, int k, int x, int y, int z) {
	int temp[6];
	for (int l = 0; l < 6; l++) {
		temp[l] = cube[i][j][k][l];
		cube[i][j][k][l] = cube[x][y][z][l];
		cube[x][y][z][l] = temp[l];
	}
}

void Cube::rotate(int x, int y, int z) {
	vector<int> dirs = getSides(make_tuple(x,y,z));
	int temp = cube[x][y][z][dirs[0]];
	for (int i = 0; i < dirs.size()-1; i++) {
		cube[x][y][z][dirs[i]] = cube[x][y][z][dirs[i+1]];
	}
	cube[x][y][z][dirs[dirs.size()-1]] = temp;
}

bool Cube::checkSolved() {
	int positions[6][9];
	getPositions(positions);
	for (int i = 0; i < 6;  i++) {
		int colour = positions[i][0];
		for (int j = 0; j < 9; j++) {
			if (positions[i][j] != colour) {
				return false;
			}
		}
	}
	return true;
}