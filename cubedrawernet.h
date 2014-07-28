#pragma once

#include <opencv2/core/core.hpp>

using namespace cv;
using namespace std;

#include "cubedrawer.h"

class CubeDrawerNet : CubeDrawer {

protected:

	// Returns the start point of the face with index index
	Point getStartPoint(int index, int height);
	
public:

	CubeDrawerNet();
	CubeDrawerNet(int height);
	
	// Draws the net of a cube
	Mat drawCubeOutline(int height);

	// Colours in sticker_index on face_index with colour on image
	Mat colourSticker(int face_index, int sticker_index, Scalar& colour);

	// Colours in face with index index with the colours in value.
	Mat colourFace(int index, Scalar* values);

	// Colours cube
	Mat colourCube(int positions[6][9], Scalar* values);

};