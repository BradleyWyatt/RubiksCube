#pragma once

#include <utility>

using namespace std;

class Position {

private:
	
	int default_x;
	int default_y;

	vector<vector<pair<int,int>>> positions;
	void updatePositions(int x, int y);
	void writeToFile(int x, int y);

public:

	Position();
	void getPositionFromFile();
	void changePositions();
	vector<vector<pair<int, int>>> getPositions();

};
