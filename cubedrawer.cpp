#include "stdafx.h"

#include <opencv2/core/core.hpp>

#include "cubedrawer.h"

Mat CubeDrawer::drawFace(Point start) {
	int section = image.rows/10;
	int x = start.x;
	int y = start.y;
	line(image, start, Point(x+3*section, y), Scalar(0,0,0), 3);
	line(image, Point(x, y+section), Point(x+3*section, y+section), Scalar(0,0,0));
	line(image, Point(x, y+2*section), Point(x+3*section, y+2*section), Scalar(0,0,0));
	line(image, Point(x, y+3*section), Point(x+3*section, y+3*section), Scalar(0,0,0), 3);
	line(image, start, Point(x, y+3*section), Scalar(0,0,0), 3);
	line(image, Point(x+section, y), Point(x+section, y+3*section), Scalar(0,0,0));
	line(image, Point(x+2*section, y), Point(x+2*section, y+3*section), Scalar(0,0,0));
	line(image, Point(x+3*section, y), Point(x+3*section, y+3*section), Scalar(0,0,0), 3);
	return image;
}