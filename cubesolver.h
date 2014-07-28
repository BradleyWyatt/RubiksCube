#pragma once

#include <iostream>
#include <tuple>
#include <vector>

#include <opencv2/core/core.hpp>

#include "cube.h"

using namespace cv;
using namespace std;

class CubeSolver {

private:
	
	// Last dimension: x, -x, y, -y, z, -z
	Cube* cube;
	Scalar colours[6];

	static const int move_limit = 300;

	// The updated stickers on the Rubik's Cube
	vector<int> updated;

	// Calculates the correctness score of the 6 sides. Stores the rotations for the optimal position for each side in rotations.
	void calcCorrectnessScore(int correctness_score[6], int rotations[6]);

	// Returns the number of corner matches of side side
	int numCornerMatches(int side);

	// Returns the number of matches for each side, given the colours of the side pieces, and the number of rotations needed for the max number of matches
	int numSideMatches(int side, int side_piece_neighbours[4], int& num_rotations);

	// Returns the side with colour colour
	int findSide(int colour);

	// Returns the number of clockwise rotations the cube (viewed from above) to make side face right
	int getExtraRotations(int side);

	// Finds the left or right side pieces (x = 0 or 2) which have colour facing front, back, up or down
	void findLeftRight(int colour, tuple<int,int,int> locations[4], bool left);

	// Moves the left or right side pieces into place to make the cross on the first layer of the cube
	void moveLeftRightIntoPlace(tuple<int,int,int> locations[4], int correctness_score[6], int rotations[6], bool left);

	// Checks if a corner is out of place. Only considers certain colours corners if that parameter is given
	bool checkCorner(int x, int y, int z, int colour=-1);

	// Finds an out-of-place corner
	tuple<int,int,int> findCorner(int colour);

	// Finds the correct positions of the given corner
	tuple<int,int,int> findCorrectPosition(tuple<int,int,int> corner);

	// Returns the number of clockwise rotations about 4 required to get [a][b][z] = [2][2][z];
	int getIndex(int a, int b);

	bool checkEdge(int x, int y, int z, int colour);

	// Finds the edge in the top layer of the Rubik's Cube which has an edge of the given colour
	tuple<int,int,int> findEdge(int colour);

	// Finds the face which has the highest correctness score
	int findBestScore(int correctness_score[6], int& score);

	// Finds the number of rotations to move the given edge to the front of the cube
	int rotationsToFront(tuple<int,int,int> edge);

	// Does the uldr number times. Terminates early if [2][2][0] is positioned correctly
	void uldrAlgorithm1(int number=6);

	// Does uldr algorithm
	void uldrAlgorithm();

	// Does urdl algorithm
	void uldrAlgorithmR();

	// Does level 2 algorithm
	void layerTwoAlgorithm(bool left);

	// Moves the top layer edges into place once the cross is made
	void moveTopLayerEdges();

	// Creates a cross on one side of the cube
	void solveLayerOneCross();

	// Moves layer one corners into place
	void solveLayerOneCorners();

	// Solves layer two
	void solveLayerTwo();

	// Solves layer three
	void solveLayerThreeCross();

	// Gets the cross on layer three
	void getLayerThreeCross();

	// Switches the pieces on the edge of the cross and orientates
	void orientateLayerThreeCross();

	// Performs the uldr algorithm to solve the cube
	void uldrAlgorithmFinal();

	// Performs the algorithm to swap the corners
	void cornerAlgorithm();

	// Returns the number of rotations required to move the only correct corner to the front right of the cube. If more than 1 are correct, returns -1
	int checkLayerThreeCorners();

	// Checks a single corner of layer three
	bool checkLayerThreeCorner(int x, int y, int z);

	// Moves the layer three corners into place
	void solveLayerThreeCorners();

	// Finds corner value of the cube at location (i, j, k) for use in checking whether the cube is solvable
	int cornerValue(int x, int y, int z);

	// Finds edge value of the cube at location (i, j, k) for use in checking whether the cube is solvable
	int edgeValue(int x, int y, int z);

public:

	// Constructor, passing the Cube which the CubeSolver is to solve.
	CubeSolver(Cube* cube);

	// Solves cube. Returns true if solvable
	bool solve();

	// Returns whether the cube is solvable
	bool checkSolvable();

};