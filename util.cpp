#include "stdafx.h"
#include <cmath>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "util.h"

using namespace cv;
using namespace std;

#define INF 999999

namespace util {

	Mat frameDifference(Mat prev, Mat current) {
		Mat diff;
		absdiff(current, prev, diff);
		threshold(diff, diff, 20, 200, CV_THRESH_BINARY);
		return diff;
	}

	Mat meanFilter(vector<Mat> prev, Mat current) {
		Mat total = prev[0].clone();
		for (int i = 0; i < prev.size(); i++) {
			total += prev[i];
		}
		Mat diff;
		absdiff(current, total, diff);
		threshold(diff, diff, 40, 200, CV_THRESH_BINARY);
		return diff;
	}

	double length(const Vec4i& line) {
		return length(line[0], line[1], line[2], line[3]);
	}

	double length(int x1, int y1, int x2, int y2) {
		int x = x1-x2;
		int y = y1-y2;
		return sqrt(x*x+y*y);
	}

	double gradient(Vec4i* line) {
		Vec4i& l = *line;
		if (l[0] == l[2]) {
			return INF;
		} else {
			return (l[3]-l[1])/(l[2]-l[0]);
		}
	}

	bool endPoint(const Vec4i& ab, const Vec4i& cd) {
		return ((ab[0] == cd[0] && ab[1] == cd[1]) ||
			(ab[0] == cd[2] && ab[1] == cd[3]) ||
			(ab[2] == cd[0] && ab[3] == cd[1]) ||
			(ab[2] == cd[2] && ab[3] == cd[3]));
	}

	bool intersect(const Vec4i& ab, const Vec4i& cd) {
		int x0 = ab[0];
		int y0 = ab[1];
		int x1 = ab[2];
		int y1 = ab[3];
		int x2 = cd[0];
		int y2 = cd[1];
		int x3 = cd[2];
		int y3 = cd[3];
		int a = y1 - y0;
		int b = x0 - x1;
		int c = x1*y0 - x0*y1;
		int k1 = a*x2 + b*y2 + c;
		int k2 = a*x3 + b*y3 + c;
		if (k1 * k2 > 0) {
			return false;
		}
		a = y3 - y2;
		b = x2 - x3;
		c = x3*y2 - x2*y3;
		k1 = a*x0 + b*y0 + c;
		k2 = a*x1 + b*y1 + c;
		if (k1 * k2 > 0) {
			return false;
		}
		return true;
	}

	double minDistance(Vec4i& ab, Vec4i& cd, bool do_clip, int threshold) {
		double min;
		int xa = ab[0];
		int ya = ab[1];
		int xb = ab[2];
		int yb = ab[3];
		int xc = cd[0];
		int yc = cd[1];
		int xd = cd[2];
		int yd = cd[3];
		int xab = xa-xb;
		int xac = xa-xc;
		int xad = xa-xd;
		int xbc = xb-xc;
		int xcd = xc-xd;
		int yab = ya-yb;
		int yac = ya-yc;
		int yad = ya-yd;
		int ybc = yb-yc;
		int ycd = yc-yd;
		double absq = (xab*xab)+(yab*yab);
		double cdsq = (xcd*xcd)+(ycd*ycd);
		double s = ((double)((xab*xac)+(yab*yac)))/absq;

		int index = 0;
		double d;
		min = distance(s, xa, ya, xb, yb, xc, yc);

		s = ((double)((xab*xad)+(yab*yad)))/absq;
		d = distance(s, xa, ya, xb, yb, xd, yd);
		if (d < min) {
			min = d;
			index = 1;
		}

		s = ((double)((xcd*-xac)+(ycd*-yac)))/cdsq;
		d = distance(s, xc, yc, xd, yd, xa, ya);
		if (d < min) {
			min = d;
			index = 2;
		}

		s = ((double)((xcd*-xbc)+(ycd*-ybc)))/cdsq;
		d = distance(s, xc, yc, xd, yd, xb, yb);
		if (d < min) {
			min = d;
			index = 3;
		}

		if (do_clip && min <= threshold) {
			switch (index) {
			case 0:
				clip(ab, cd, 0);
				break;
			case 1:
				clip(ab, cd, 1);
				break;
			case 2:
				clip(cd, ab, 0);
				break;
			case 3:
				clip(cd, ab, 1);
			}
		}
		return min;
	}

	void clip(const Vec4i& ab, Vec4i& cd, int side) {
		if (ab[0] != ab[2] && cd[0] != cd[2]) {
			double m1 = ((double)(ab[3]-ab[1]))/(ab[2]-ab[0]);
			double m2 = ((double)(cd[3]-cd[1]))/(cd[2]-cd[0]);
			int x1 = ab[0];
			int y1 = ab[1];
			int* x2;
			int* y2;
			if (side == 0) {
				x2 = &cd[0];
				y2 = &cd[1];
			} else {
				x2 = &cd[2];
				y2 = &cd[3];
			}
			double t = (m1*x1 - y1 - m2*(*x2) + (*y2))/(m1-m2);
			*x2 = t;
			*y2 = m1*(t) - m1*x1 + y1;
		} else if (ab[0] == ab[2]) {
			int* y2;
			int dx;
			if (side == 0) {
				dx = cd[0] - ab[0];
				cd[0] = ab[0];
				y2 = &cd[1];
			} else {
				dx = cd[2] - ab[0];
				cd[2] = ab[0];
				y2 = &cd[3];
			}
			double m = ((double)(cd[3]-cd[1]))/(cd[2]-cd[0]);
			*y2 -= m*dx;
		} else {
			int* y2;
			int dx = ab[0] - cd[0];
			double m = ((double)(ab[3]-ab[1]))/(ab[2]-ab[0]);
			if (side == 0) {
				dx = cd[0] - ab[0];
				y2 = &cd[1];
			} else {
				dx = cd[2] - ab[0];
				y2 = &cd[3];
			}
			*y2 += m*dx;
		}
	}

	double distance(double s, int x1, int y1, int x2, int y2, int x3, int y3) {
		if (s < 0) {
			return length(x1,y1,x3,y3);
		} else if (s > 1) {
			return length(x2,y2,x3,y3);
		} else {
			double x = (1-s)*x1 + s*x2;
			double y = (1-s)*y1 + s*y2;
			return length(x,y,x3,y3);
		}
	}

	Scalar median(Mat& image, Mat& mask) {
		MatIterator_<Vec3b> it = image.begin<Vec3b>();
		MatIterator_<uchar> mask_it = mask.begin<uchar>();
		MatIterator_<uchar> end = mask.end<uchar>();
		vector<double> hue_values;
		double boundary_count = 0;
		double s = 0;
		double v = 0;
		for (; mask_it != end; it++, mask_it++) {
			if (*mask_it > 0) {
				// If the hue value is near the boundary
				if ((*it)[0] < 45 || (*it)[0] > 135) {
					boundary_count++;
				}
				s += (*it)[1];
				v += (*it)[2];
				hue_values.push_back(Scalar(*it)[0]);
			}
		}
		// If 75% of the hue values are near the boundary
		if (boundary_count/hue_values.size() > 0.75) {
			for (double& hue : hue_values) {
				if (hue < 90) {
					hue += 180;
				}
			}
		}
		sort(hue_values.begin(), hue_values.end());
		if (boundary_count/hue_values.size() > 0.75) {
			for (double& hue : hue_values) {
				if (hue > 180) {
					hue -= 180;
				}
			}
		}
		int size = hue_values.size();
		if (size == 0) {
			return Scalar(0,0,0);
		}
		return Scalar(hue_values[size/2], s/size, v/size);
	}

}