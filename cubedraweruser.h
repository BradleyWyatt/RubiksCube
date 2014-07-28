#pragma once

#include <opencv2/core/core.hpp>

#include "cubedrawer.h"

using namespace cv;
using namespace std;

class CubeDrawerUser : CubeDrawer {

protected:
	
	// Returns the start point of the face with index index
	Point getStartPoint(int index, int height);

	// Colours in sticker_index on face_index with colour on image
	Mat colourSticker(int face_index, int sticker_index, Scalar& colour);

public:
	
	CubeDrawerUser();
	CubeDrawerUser(int height);
	~CubeDrawerUser();

	// Colours cube
	Mat colourCube(int positions[6][9], Scalar* values);

	void updateCube(int positions[6][9], Scalar colours[6], vector<vector<pair<int, int>>>& window_positions);

	// Draws the net of the cube
	Mat drawCubeOutline(int height);

};