#include "stdafx.h"

#include <fstream>
#include <iostream>
#include <thread>

#include <Windows.h>

#include <opencv2/core/core.hpp>
#include <opencv2/core/operations.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "colourgrouper.h"
#include "cube.h"
#include "cubedrawernet.h"
#include "cubesolver.h"

#include "evaluation.h"

using namespace cv;
using namespace std;

namespace evaluation {

	void generateFFmpeg() {
		string fname = "ffmpeg.txt";
		ofstream myfile;
		srand(time(0));
		myfile.open (fname, std::ofstream::out | std::ofstream::app);
		for (int i = 1; i <= 7; i++) {
			for (int j = 0; j < 10; j++) {
				int arr[9] = {0,0,0,0,0,0,0,0,0};
				while (arr[0] + arr[1] + arr[2] != 15) {
					arr[0] = rand()%10;
					arr[1] = rand()%10;
					arr[2] = rand()%10;
				}
				while (arr[3] + arr[4] + arr[5] != 15) {
					arr[3] = rand()%10;
					arr[4] = rand()%10;
					arr[5] = rand()%10;
				}
				while (arr[6] + arr[7] + arr[8] != 15) {
					arr[6] = rand()%10;
					arr[7] = rand()%10;
					arr[8] = rand()%10;
				}
				myfile << "ffmpeg -i rubiks" << i << ".avi -vf colorchannelmixer=." << to_string(arr[0])
					<< ":." << to_string(arr[3]) << ":." << to_string(arr[6]) << ":0:." << to_string(arr[1])
					<< ":." << to_string(arr[4]) << ":." << to_string(arr[7]) << ":0:." << to_string(arr[2])
					<< ":." << to_string(arr[5]) << ":." << to_string(arr[8]) << ": rubiks" << i*10+j <<".avi & ";
			}
		}
		myfile.close();
	}

	void processData() {
		cout << "Enter filename" << endl;
		string filename;
		cin >> filename;
		ifstream myfile(filename);
		int results[4] = {0,0,0,0};
		int combined = 0;
		int c = 0;
		if (myfile.is_open()) {
			while (!myfile.eof()) {
				c++;
				int total = 0;
				for (int i = 0; i < 4; i++) {
					int val;
					myfile >> val;
					cout << val << " ";
					results[i] += val;
					total += val;
				}
				if (total > 0) {
					combined++;
				}
				cout << endl;
				cout << "Line count = " << c << endl;
				for (int i = 0; i < 4; i++) {
					cout << results[i] << " ";
				}
				cout << endl;
			}
			myfile.close();
		}
		cout << "Combined score = " << combined << endl;
		// To keep console open
		cin >> filename;
	}

	void solver(int num_trials, bool solvable) {
		int parity_success = 0;
		int parity_fail = 0;
		int test_success = 0;
		int test_fail = 0;
		int match = 0;
		srand(GetTickCount());
		long start = time(0);
		/*int positions[6][9];
		ifstream myfale("f.txt");
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 9; j++) {
				myfale >> positions[i][j];
			}
		}		
		Scalar colours[6] = {Scalar(255,0,0), Scalar(0,255,0), Scalar(0,0,255), Scalar(255,255,0), Scalar(255,0,255), Scalar(0,255,255)};
		Cube cube(positions, colours);
		CubeDrawerNet n(200);
		imshow("C", n.colourCube(positions, colours));
		CubeSolver cs(&cube);
		cout << cs.solve() << endl;
		cube.replayMoves(positions, colours);
		return;*/
		for (int i = 0; i < num_trials; i++) {
			if (i % 100000 == 0) {
				cout << "Trial: " << i << endl;
				ofstream myfile;
				string fname = "evaluation_solving" + to_string(solvable) + ".txt";
				myfile.open(fname, std::ofstream::out | std::ofstream::app);
				myfile << "Stats at trial " << i << endl;
				myfile << "Number of trials: " << num_trials << endl;
				myfile << match << " matches" << endl;
				myfile << "Parity stats: " << parity_success << " " << parity_fail << endl;
				myfile << "Test stats: " << test_success << " " << test_fail << endl;
				myfile << endl;
				myfile.close();
			}
			int positions[6][9];
			for (int j = 0; j < 6; j++) {
				for (int k = 0; k < 9; k++) {
					positions[j][k] = j;
				}
			}
			Scalar colours[6] = {Scalar(255,0,0), Scalar(0,255,0), Scalar(0,0,255), Scalar(255,255,0), Scalar(255,0,255), Scalar(0,255,255)};
			Cube cube(positions, colours);
			
			if (!solvable) {
				int a = 1;
				int b = 1;
				int c = 1;
				while (a == 1 && b == 1 && c == 1 || !((a%2 == 0 && b%2 == 0 && c%2 == 0) || ((a + b + c) % 2 == 1))) {
					a = rand() % 3;
					b = rand() % 3;
					c = rand() % 3;
				}
				cube.rotate(a,b,c);
			}

			int num_moves = rand() % 6;
			for (int j = 0; j < 15+num_moves; j++) {
				int side = rand() % 6;
				int num_rotations = 1 + (rand() % 3);
				cube.rotateLayer(side, 0, num_rotations, false);
			}
			cube.getPositions(positions);
			
		//	CubeDrawerNet n(200);
		//	imshow("C", n.colourCube(positions, colours));

			Cube to_solve(positions, colours);
			CubeSolver solver(&to_solve);
			bool solvable_parity = solver.checkSolvable();
			bool solvable_testing = solver.solve();
		//	waitKey(0);
		//	if (!solvable_testing) {
		//		to_solve.loadPositions(positions);
		//		to_solve.rotateCube(1, 1, false);
		//		to_solve.rotateCube(3, 1, false);
		//		to_solve.decodeDirections();
		//	}
			if (solvable_parity == solvable_testing) {
				match++;
			} else {
				ofstream myfile;
				string fname = "evaluation_solving_fail1.txt";
				myfile.open(fname, std::ofstream::out | std::ofstream::app);
				for (int x = 0; x < 6; x++) {
					for (int y = 0; y < 9; y++) {
						myfile << positions[x][y] << " ";
					}
					myfile << endl;
				}
				myfile << endl;
				myfile.close();
			}
			if (solvable_testing) {
				test_success++;
			} else {
				test_fail++;
			}
			if (solvable_parity) {
				parity_success++;
			} else {
				parity_fail++;
			}
		}
		long end = time(0);
		ofstream myfile;
		string fname = "evaluation_solving" + to_string(solvable) + ".txt";
		myfile.open(fname, std::ofstream::out | std::ofstream::app);
		myfile << "==============" << endl;
		myfile << "Number of trials: " << num_trials << endl;
		myfile << match << " matches" << endl;
		myfile << "Parity stats: " << parity_success << " " << parity_fail << endl;
		myfile << "Test stats: " << test_success << " " << test_fail << endl;
		myfile << "Total time: " << end-start << endl;
		myfile << endl;
		myfile.close();
	}

	void colours(Scalar hsv_values[6][9], string id) {
		Scalar values[4][6][9];
		Scalar values_copy[4][6][9];
		
		CubeDrawerNet cube_drawer(200);
			Mat cube_imagea;
			for (int i = 0; i < 6; i++) {
				for (int j = 0; j < 9; j++) {
					cube_imagea = cube_drawer.colourSticker(i, j, values[2][i][j]);
				}
			}
		imshow("Cube grid", cube_imagea);
		moveWindow("Cube grid", 500, 150);

		int num_covered = 1;
		srand(GetTickCount());
		
		vector<int> done;
		for (int covered = 0; covered < num_covered; covered++) {
			bool completed = false;
			while (covered != 0 && !completed) {
				int sticker = rand() % 48;
				int face = sticker/8;
				int index = sticker%8;
				if (index >= 4) {
					index++;
				}
				if (find(done.begin(), done.end(), sticker) == done.end()) {
					done.push_back(sticker);
					hsv_values[face][index] = Scalar(164 + rand()%5, 38 + rand()%5, 192 + rand()%9);
					completed = true;
				}
			}

			/////////////// Changing colour space
			for (int i = 0; i < 6; i++) {
				for (int j = 0; j < 9; j++) {
					values[0][i][j] = hsv_values[i][j];
					values[1][i][j] = hsv_values[i][j];
					Mat m1(Size(1,1), CV_8UC3, hsv_values[i][j]);
					Mat m2;
					cvtColor(m1, m2, CV_HSV2BGR);
					Vec3b pixel = m2.at<Vec3b>(0,0);
					values[2][i][j] = Scalar(pixel[0], pixel[1], pixel[2]);
					cvtColor(m2, m2, CV_BGR2Lab);
					pixel = m2.at<Vec3b>(0,0);
					values[3][i][j] = Scalar(pixel[0], pixel[1], pixel[2]);
				}
			}
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 6; j++) {
					for (int k = 0; k < 9; k++) {
						values_copy[i][j][k] = values[i][j][k];
					}
				}
			}

			int num_attempts = 0;
			bool constraints_met[4];

			double max_difference = 50;
			int white_side = -1;
			for (int i = 0; i < 6; i++) {
				if (max_difference < hsv_values[i][4][2] - hsv_values[i][4][1]) {
					white_side = i;
					max_difference = hsv_values[i][4][2] - hsv_values[i][4][1];
				}
			}

			CubeDrawerNet cube_drawer(200);
			Mat cube_image;
			for (int i = 0; i < 6; i++) {
				for (int j = 0; j < 9; j++) {
					cube_image = cube_drawer.colourSticker(i, j, values[2][i][j]);
				}
			}
			imshow("Input cube1", cube_image);
			moveWindow("Input cube1", 500, 400);

			bool ever_met[4] = {false, false, false, false};

			while (num_attempts < 10) {
				num_attempts++;

				Scalar centres[4][6];
				Scalar centre_colours[4][6];
				int positions[4][6][9];
				ColourGrouper colour_grouper;
				Mat cube_image;

				colour_grouper.groupColours(cube_image, values[0], positions[0], centre_colours[0], white_side);
				colour_grouper.groupColours(cube_image, values[1], positions[1], centre_colours[1], true);
				colour_grouper.groupColours(cube_image, values[2], positions[2], centre_colours[2], false);
				colour_grouper.groupColours(cube_image, values[3], positions[3], centre_colours[3], false);

				for (int i = 0; i < 6; i++) {
					//cout << centre_colours[1][i] << endl << centre_colours[2][i] << endl << centre_colours[3][i] << endl << endl;
					Mat m1(Size(1,1), CV_8UC3, centre_colours[1][i]);
					Mat m2;
					cvtColor(m1, m2, CV_HSV2BGR);
					Vec3b pixel = m2.at<Vec3b>(0,0);
					centre_colours[1][i] = Scalar(pixel[0], pixel[1], pixel[2]);
					m1 = Mat(Size(1,1), CV_8UC3, centre_colours[3][i]);
					cvtColor(m1, m2, CV_Lab2BGR);
					pixel = m2.at<Vec3b>(0,0);
					centre_colours[3][i] = Scalar(pixel[0], pixel[1], pixel[2]);
				}
				Cube cube;
				cout << "Attempt " << num_attempts << ":";
				for (int i = 0; i < 4; i++) {
					constraints_met[i] = colour_grouper.fitColours(positions[i], values[i], centre_colours[i]);
					if (constraints_met[i]) {
						cube = Cube(positions[i], centre_colours[i]);
						CubeSolver cube_solver(&cube);
						constraints_met[i] = (cube_solver.checkSolvable() && cube_solver.solve());
					}
					ever_met[i] |= constraints_met[i];
					cout << " " << constraints_met[i];
				}
				if (num_attempts <= 10) {
					string fname = "colour_evaluation_covered_" + to_string(covered) + "_g.txt";
					ofstream myfile;
					myfile.open (fname, std::ofstream::out | std::ofstream::app);
					//myfile << id << " ";
					for (int i = 0; i < 4; i++) {
						myfile << constraints_met[i] << " ";
					}
					myfile << endl;
					if (num_attempts == 10) {
						myfile << endl;
					}
					myfile.close();
				}

				CubeDrawerNet cube_drawer_net(200);
				imshow("Weighted HSV", cube_drawer_net.colourCube(positions[0], centre_colours[0]));
				imshow("HSV", cube_drawer_net.colourCube(positions[1], centre_colours[1]));
				imshow("RGB", cube_drawer_net.colourCube(positions[2], centre_colours[2]));
				imshow("Lab", cube_drawer_net.colourCube(positions[3], centre_colours[3]));

				moveWindow("Weighted HSV", 800, 150);
				moveWindow("HSV", 1100, 150);
				moveWindow("RGB", 800, 400);
				moveWindow("Lab", 1100, 400);

				if (covered == 1) {
					for (int j = 0; j < 4; j++) {
						if (!constraints_met[j]) {
							waitKey(0);
						}
					}
				}

				waitKey(1);

				cout << endl;
				if (num_attempts == 10) {
					string fname = "colour_evaluation_covered_" + to_string(covered) + ".txt";
					ofstream myfile;
					myfile.open(fname, std::ofstream::out | std::ofstream::app);
					//myfile << id << " ";
					for (int i = 0; i < 4; i++) {
						myfile << ever_met[i] << " ";
					}
					myfile << endl;
					myfile.close();
				/*	for (int j = 0; j < 4; j++) {
						fname = "colour_evaluation_extreme_unsolved_" + to_string(covered) + "_";
						switch (j) {
						case 0: 
							fname += "hsvw";
							break;
						case 1: 
							fname += "hsv";
							break;
						case 2: 
							fname += "rgb";
							break;
						case 3: 
							fname += "lab";
							break;
						}
						fname += ".txt";
						ofstream myfile;
						myfile.open(fname, std::ofstream::out | std::ofstream::app);
						//myfile << id << " ";
						string in;
						cin >> in;
						if (in == "p") {
							myfile << "9 9 9 9 9 9";
						} else {
							myfile << in << " ";
							for (int k = 1; k < 6; k++) {
								cin >> in;
								myfile << in << " ";
							}
						}
						myfile << endl;
						cout << endl;
					}*/
				}
				if (num_attempts >= 10) {
					//break;
					/*int a = waitKey(0);
					if (a == 32) {
						break;
					}*/
				}

				for (int i = 0; i < 4; i++) {
					for (int j = 0; j < 6; j++) {
						for (int k = 0; k < 9; k++) {
							values[i][j][k] = values_copy[i][j][k];
						}
					}
				}
			}
		}
	}
}