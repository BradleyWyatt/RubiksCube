#pragma once

#include <opencv2/core/core.hpp>

using namespace cv;
using namespace std;

class CubeDrawer {

protected:

	Mat image;
	
	// Returns the start point of the face with index index
	virtual Point getStartPoint(int index, int height) = 0;

	// Draws a single face of the cube
	Mat drawFace(Point start);

	// Draws the net of a cube
	virtual Mat drawCubeOutline(int height) = 0;

	// Colours in sticker_index on face_index with colour on image
	virtual Mat colourSticker(int face_index, int sticker_index, Scalar& colour) = 0;

	// Colours cube
	virtual Mat colourCube(int positions[6][9], Scalar* values) = 0;

};