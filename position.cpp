#include "stdafx.h"

#include <fstream>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "position.h"
#include "stringnames.h"

Position::Position() {
	default_x = 567;
	default_y = 150;
	vector<pair<int,int>> col1;
	col1.push_back(make_pair(default_x, default_y));
	col1.push_back(make_pair(default_x, default_y + 250));
	positions.push_back(col1);
	vector<pair<int,int>> col2;
	col2.push_back(make_pair(default_x + 234, default_y));
	col2.push_back(make_pair(default_x + 234, default_y + 250));
	positions.push_back(col2);
	vector<pair<int,int>> col3;
	col3.push_back(make_pair(default_x + 534, default_y));
	col3.push_back(make_pair(default_x + 534, default_y + 250));
	positions.push_back(col3);
}

void Position::updatePositions(int x, int y) {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			positions[i][j].first += x;
			positions[i][j].second += y;
		}
	}
}

void Position::writeToFile(int x, int y) {
	ofstream settings_file;
	settings_file.open(position_file);
	settings_file << (positions[0][0].first + x - default_x) << " " << (positions[0][0].second + y - default_y) << endl;
	settings_file.close();
	updatePositions(x, y);
}

void Position::getPositionFromFile() {
	ifstream settings_file(position_file);
	bool success = false;
	if (settings_file.good()) {
		success = true;
		int x;
		int y;
		settings_file >> x;
		settings_file >> y;
		updatePositions(x, y);
	} else {
		writeToFile(0, 0);
		updatePositions(0, 0);
	}
	settings_file.close();
}

void Position::changePositions() {
	int x = 0;
	cout << update_x << endl;
	while (!(cin >> x)) {
		cout << enter_integer << endl;
		cout << update_x << endl;
		cin.clear();
		cin.ignore(1000, '\n');
	}
	int y = 0;
	cout << update_y << endl;
	while (!(cin >> y)) {
		cout << enter_integer << endl;
		cout << update_y << endl;
		cin.clear();
		cin.ignore(1000, '\n');
	}
	updatePositions(x, y);
	writeToFile(x, y);
}

vector<vector<pair<int,int>>> Position::getPositions() {
	return positions;
}