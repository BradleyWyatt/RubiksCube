Rubik's Cube Assistant Using Video

Demonstation available at: http://youtu.be/FhC0A6j_v6Y

To commemorate the 40th anniversary of the invention of the Rubik's Cube I created a Rubik's Cube Assistant. It is implemented in C++ and I used some functionality of the OpenCV library.

It is initialised by showing each face in turn to the webcam, and then gives instructions on how to solve the Rubik's Cube, only advancing when the correct move is made. It works in a range of conditions - distance from the camera, partial occlusions by fingers, different backgrounds and with any colour layout.

Making this into a mobile phone app is left for future work and I encourage anyone to fork it for that purpose.

Brief overview:

Detects the Rubik's Cube using the aid of OpenCV functionality to extract straight lines from the image. Once the Rubik's Cube is detected, it is tracked so that the detection does not need to be continuously repeated.
For each face, extracts the colour of each sticker of the Rubik's Cube by taking the median colour of the regions where the position of the stickers are calculated.
Uses a restricted k-means clustering algorithm (k = 6, each cluster restricted to exactly 9 elements) to group the stickers into sets of the same colour. This is performed in RGB, HSV and Lab colour spaces.
After the clustering, checks if the clustering is valid - not all Rubik's Cubes are solvable. If not, tries to solve as constraint satisfaction problem. The constraints are added in increasing order of distance of its colour from the cluster centre.
Calculates a solution to the Rubik's Cube.
Issue instructions to the user, and checks the median colour of each sticker to check if the correct move has been made. If it has, issues next instruction; if not, checks again next frame. Throughout this, also calculates the user's skin colour by finding the mean and variance of regions of stickers where the median sticker colour is as expected, but other regions are not - This is likely to be skin. Skin colour can be ignored in future when calculating the median colour.
