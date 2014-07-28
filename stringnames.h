#pragma once

#include <string>

using namespace std;

// Camera Calibration

// File to store calibration settings
const string calibration_file = "camera_calibration_data.txt";

const string chessboard_width = "Enter the width of the chessboard:";
const string chessboard_height = "Enter the height of the chessboard:";
const string enter_integer = "Please make sure that you input an integer higher than 1!";
const string calibrated_message = "Calibration completed!";

// Window positions
const string position_file = "position.txt";
const string update_x = "Please enter the x offset for the new position";
const string update_y = "Please enter the y offset for the new position";

// Window names
const string input_cube_string = "Input cube";
const string detected_cube_string = "Detected cube";
const string cube_grid_string = "Cube grid";

const string cube_net = "Cube net";
const string current_user_view = "Current user view";
const string next_user_view = "Next user view";
const string instruction = "Next move";

const string solver = "Solver";

const string calibration_window = "Calibrating";

// Messages to user
const string closing_message = "Closing...";
const string completed_message = "Completed! Well done!";
const string failed_initialisation_message = "Failed to initialise cube. Please input again";
const string space_to_retry_message = "Press space to try again, or anything else to quit";
const string solution_length = "Length of solution: ";

// Instructions whilst solving the cube
const string rotate_cube_message = "Rotate the entire cube by rotating ";
const string side_0_rotate_cube_message = "bottom side towards you 90 degrees";
const string side_1_rotate_cube_message = "top side towards you 90 degrees";
const string side_2_rotate_cube_message = "Rubik's Cube 90 degrees clockwise";
const string side_3_rotate_cube_message = "Rubik's Cube 90 degrees anticlockwise";
const string side_4_rotate_cube_message = "right side towards you 90 degrees";
const string side_5_rotate_cube_message = "left side towards you 90 degrees";
const string rotate_message = "Rotate the ";
const string right_message = "right";
const string left_message = "left";
const string front_message = "front";
const string back_message = "back";
const string top_message = "top";
const string bottom_message = "bottom";
const string rotate_face_message = " face 90 degrees ";
const string clockwise_message = "clockwise";
const string anticlockwise_message = "anticlockwise";

// My name
const string name = 
	"Bradley Wyatt\n"
	"University of Cambridge\n"
	"Part II Project\n"
	"\n";

// Instructions shown upon starting
const string instructions = 
	 "How to use:\n"
	 "Show a face to the camera\n"
	 "Press space once the tool has locked onto the cube and drawn a green outline around it\n"
	 "Press backspace if the tool has not detected reasonable colours\n"
	 "Press space once all 6 faces have been detected, otherwise press backspace to remove all faces\n"
	 "After each face is drawn on the screen, check that the orientation and colours are correct\n"
	 "After all 6 faces have been entered, press space to start guidance\n"
	 "Diagrams are given as well as the text prompt for each stage\n"
	 "\n"
	 "Order to show the faces:\n"
	 "Show any face and capture cube\n"
	 "Rotate the left side of the face towards you and capture cube (3 times)\n"
	 "Rotate the bottom side of the face towards you and capture cube\n"
	 "Rotate the bottom side of the face towards you 180 degrees and capture cube\n"
	 "\n"
	 "Terminology:\n"
	 "When an instruction is given, a clockwise/anticlockwise rotation of a face is the direction whilst looking at that face of the cube\n"
	 "When told to rotate the cube clockwise/anticlockwise without any face/side specified, assume the front face\n"
	 "\n"
	 "Press 'c' if the camera needs calibrating\n"
	 "Press 'p' to reposition the default location of the windows\n";
