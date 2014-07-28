#include "stdafx.h"
#include <cmath>

#define NDEBUG

#include <opencv2/core/core.hpp>
#include <opencv2/core/operations.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "circular.h"
#include "cubedraweruser.h"
#include "stringnames.h"

#define INF 999999

CubeDrawerUser::CubeDrawerUser() {}

CubeDrawerUser::CubeDrawerUser(int height) {
	drawCubeOutline(height);
}

CubeDrawerUser::~CubeDrawerUser() {
	destroyWindow(current_user_view);
	destroyWindow(next_user_view);
}

Mat CubeDrawerUser::drawCubeOutline(int height) {
	image = Mat(Size(4.0/3*height, height), CV_8UC3, Scalar(255,255,255));
	int section = height/20;
	int start = (image.cols-image.rows)/2;
	// Horizontal face edges
	line(image, Point(start+7*section, section), Point(start+13*section, section), Scalar(0,0,0), 3);
	line(image, Point(start+section, 7*section), Point(start+19*section, 7*section), Scalar(0,0,0), 3);
	line(image, Point(start+section, 13*section), Point(start+19*section, 13*section), Scalar(0,0,0), 3);
	line(image, Point(start+7*section, 19*section), Point(start+13*section, 19*section), Scalar(0,0,0), 3);
	
	// Horizontal sticker edges
	line(image, Point(start+7*section, 3*section), Point(start+13*section, 3*section), Scalar(0,0,0));
	line(image, Point(start+7*section, 5*section), Point(start+13*section, 5*section), Scalar(0,0,0));
	line(image, Point(start+section, 9*section), Point(start+19*section, 9*section), Scalar(0,0,0));
	line(image, Point(start+section, 11*section), Point(start+19*section, 11*section), Scalar(0,0,0));
	line(image, Point(start+7*section, 15*section), Point(start+13*section, 15*section), Scalar(0,0,0));
	line(image, Point(start+7*section, 17*section), Point(start+13*section, 17*section), Scalar(0,0,0));

	// Vertical face edges
	line(image, Point(start+section, 7*section), Point(start+section, 13*section), Scalar(0,0,0), 3);
	line(image, Point(start+7*section, section), Point(start+7*section, 19*section), Scalar(0,0,0), 3);
	line(image, Point(start+13*section, section), Point(start+13*section, 19*section), Scalar(0,0,0), 3);
	line(image, Point(start+19*section, 7*section), Point(start+19*section, 13*section), Scalar(0,0,0), 3);

	// Vertical sticker edges
	line(image, Point(start+3*section, 7*section), Point(start+3*section, 13*section), Scalar(0,0,0));
	line(image, Point(start+5*section, 7*section), Point(start+5*section, 13*section), Scalar(0,0,0));
	line(image, Point(start+9*section, section), Point(start+9*section, 19*section), Scalar(0,0,0));
	line(image, Point(start+11*section, section), Point(start+11*section, 19*section), Scalar(0,0,0));
	line(image, Point(start+15*section, 7*section), Point(start+15*section, 13*section), Scalar(0,0,0));
	line(image, Point(start+17*section, 7*section), Point(start+17*section, 13*section), Scalar(0,0,0));

	return image;
}

Mat CubeDrawerUser::colourCube(int positions[6][9], Scalar* values) {
	for (int i = 1; i < 6; i++) {
		for (int j = 0; j < 9; j++) {
			colourSticker(i, j, values[positions[i][j]]);
		}
	}
	return image;
}

Point CubeDrawerUser::getStartPoint(int index, int height) {
	int section = height/20;
	switch (index) {
	case 1: 
		return Point(13*section, 7*section);
	case 2: 
		return Point(7*section, 7*section);
	case 3: 
		return Point(section, 7*section);
	case 4: 
		return Point(7*section, section);
	case 5:
		return Point(7*section, 13*section);
	}
}

Mat CubeDrawerUser::colourSticker(int face_index, int sticker_index, Scalar& colour) {
	int offset = (image.cols-image.rows)/2;
	int section = image.rows/10;
	Point start = getStartPoint(face_index, image.rows);
	int x = start.x + offset;
	int y = start.y;
	Point points[1][4];
	int i = 2-(sticker_index % 3);
	int j = sticker_index / 3;
	if (face_index >= 4) {
		int temp = i;
		i = j;
		j = temp;
		if (face_index == 4) {
			i = 2-i;
		} else {
			j = 2-j;
		}
	}
	points[0][0] = Point(x+i*section, y+j*section);
	points[0][1] = Point(x+(i+1)*section, y+j*section);
	points[0][2] = Point(x+(i+1)*section, y+(j+1)*section);
	points[0][3] = Point(x+i*section, y+(j+1)*section);
	const Point* ppt[1] = { points[0] };
	int npt[] = { 4 };
	fillPoly(image, ppt, npt, 1, colour);
	return drawFace(Point(x,y));
}
#include <iostream>
void CubeDrawerUser::updateCube(int positions[6][9], Scalar colours[6], vector<vector<pair<int, int>>>& window_positions) {
	//Mat image;
	//imshow("Solver", image);
	moveWindow(solver, window_positions[2][1].first, window_positions[2][1].second);
	Mat current_user_view_image = image.clone();
	imshow(current_user_view, current_user_view_image);
	colourCube(positions, colours);
	imshow(next_user_view, image);
	moveWindow(current_user_view, window_positions[1][1].first, window_positions[1][1].second);
	moveWindow(next_user_view, window_positions[2][1].first, window_positions[2][1].second);
}