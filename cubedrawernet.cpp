#include "stdafx.h"
#include <cmath>

#include <opencv2/core/core.hpp>
#include <opencv2/core/operations.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "circular.h"
#include "cubedrawernet.h"

#define INF 999999

CubeDrawerNet::CubeDrawerNet() { }

CubeDrawerNet::CubeDrawerNet(int height) {
	drawCubeOutline(height);
}

Mat CubeDrawerNet::drawCubeOutline(int height) {
	image = Mat(Size(height*4/3, height), CV_8UC3, Scalar(255,255,255));
	int section = height/20;
	// Horizontal face edges
	line(image, Point(19*section, section), Point(25*section, section), Scalar(0,0,0), 3);
	line(image, Point(section, 7*section), Point(25*section, 7*section), Scalar(0,0,0), 3);
	line(image, Point(section, 13*section), Point(25*section, 13*section), Scalar(0,0,0), 3);
	line(image, Point(19*section, 19*section), Point(25*section, 19*section), Scalar(0,0,0), 3);
	
	// Horizontal sticker edges
	line(image, Point(19*section, 3*section), Point(25*section, 3*section), Scalar(0,0,0));
	line(image, Point(19*section, 5*section), Point(25*section, 5*section), Scalar(0,0,0));
	line(image, Point(section, 9*section), Point(25*section, 9*section), Scalar(0,0,0));
	line(image, Point(section, 11*section), Point(25*section, 11*section), Scalar(0,0,0));
	line(image, Point(19*section, 15*section), Point(25*section, 15*section), Scalar(0,0,0));
	line(image, Point(19*section, 17*section), Point(25*section, 17*section), Scalar(0,0,0));

	// Vertical face edges
	line(image, Point(section, 7*section), Point(section, 13*section), Scalar(0,0,0), 3);
	line(image, Point(7*section, 7*section), Point(7*section, 13*section), Scalar(0,0,0), 3);
	line(image, Point(13*section, 7*section), Point(13*section, 13*section), Scalar(0,0,0), 3);
	line(image, Point(19*section, section), Point(19*section, 19*section), Scalar(0,0,0), 3);
	line(image, Point(25*section, section), Point(25*section, 19*section), Scalar(0,0,0), 3);

	// Vertical sticker edges
	line(image, Point(3*section, 7*section), Point(3*section, 13*section), Scalar(0,0,0));
	line(image, Point(5*section, 7*section), Point(5*section, 13*section), Scalar(0,0,0));
	line(image, Point(9*section, 7*section), Point(9*section, 13*section), Scalar(0,0,0));
	line(image, Point(11*section, 7*section), Point(11*section, 13*section), Scalar(0,0,0));
	line(image, Point(15*section, 7*section), Point(15*section, 13*section), Scalar(0,0,0));
	line(image, Point(17*section, 7*section), Point(17*section, 13*section), Scalar(0,0,0));
	line(image, Point(21*section, section), Point(21*section, 19*section), Scalar(0,0,0));
	line(image, Point(23*section, section), Point(23*section, 19*section), Scalar(0,0,0));

	return image;
}

Mat CubeDrawerNet::colourSticker(int face_index, int sticker_index, Scalar& colour) {
	int section = image.rows/10;
	Point start = getStartPoint(face_index, image.rows);
	int x = start.x;
	int y = start.y;
	Point points[1][4];
	int i = sticker_index % 3;
	int j = sticker_index / 3;
	points[0][0] = Point(x+i*section, y+j*section);
	points[0][1] = Point(x+(i+1)*section, y+j*section);
	points[0][2] = Point(x+(i+1)*section, y+(j+1)*section);
	points[0][3] = Point(x+i*section, y+(j+1)*section);
	const Point* ppt[1] = { points[0] };
	int npt[] = { 4 };
	fillPoly(image, ppt, npt, 1, colour);
	return drawFace(start);
}

Mat CubeDrawerNet::colourFace(int index, Scalar* values) {
	Point start = getStartPoint(index, image.rows);
	int section = image.rows/10;
	int x = start.x;
	int y = start.y;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			Point points[1][4];
			points[0][0] = Point(x+i*section, y+j*section);
			points[0][1] = Point(x+(i+1)*section, y+j*section);
			points[0][2] = Point(x+(i+1)*section, y+(j+1)*section);
			points[0][3] = Point(x+i*section, y+(j+1)*section);
			const Point* ppt[1] = { points[0] };
			int npt[] = { 4 };
			int colour_index = i+3*(2-j);
			fillPoly(image, ppt, npt, 1, values[colour_index]);
		}
	}
	return drawFace(start);
}

Mat CubeDrawerNet::colourCube(int positions[6][9], Scalar* values) {
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 9; j++) {
			colourSticker(i, j, values[positions[i][j]]);
		}
	}
	return image;
}

Point CubeDrawerNet::getStartPoint(int index, int height) {
	int section = height/20;
	switch (index) {
	case 0: 
		return Point(section, 7*section);
	case 1: 
		return Point(7*section, 7*section);
	case 2: 
		return Point(13*section, 7*section);
	case 3: 
		return Point(19*section, 7*section);
	case 4: 
		return Point(19*section, section);
	case 5:
		return Point(19*section, 13*section);
	}
}