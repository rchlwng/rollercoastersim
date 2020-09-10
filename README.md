Subject 	: CSCI420 - Computer Graphics 
Assignment 2: Simulating a Roller Coaster
Author		: Rachel Wang
USC ID 		: 4483024455

Description: In this assignment, we use Catmull-Rom splines along with OpenGL core profile shader-based texture mapping and Phong shading to create a roller coaster simulation.

Core Credit Features: (Answer these Questions with Y/N; you can also insert comments as appropriate)
======================

1. Uses OpenGL core profile, version 3.2 or higher - 

2. Completed all Levels:
  Level 1 : - Y
  level 2 : - Y
  Level 3 : - Y
  Level 4 : - Y
  Level 5 : - Y

3. Rendered the camera at a reasonable speed in a continuous path/orientation - Y

4. Run at interactive frame rate (>15fps at 1280 x 720) - Y

5. Understandably written, well commented code - Y

6. Attached an Animation folder containing not more than 1000 screenshots - Y

7. Attached this ReadMe File - Y (very meta!)

Extra Credit Features: (Answer these Questions with Y/N; you can also insert comments as appropriate)
======================

1. Render a T-shaped rail cross section -

2. Render a Double Rail - Y

3. Made the track circular and closed it with C1 continuity -

4. Any Additional Scene Elements? (list them here) -

5. Render a sky-box -

6. Create tracks that mimic real world roller coaster -

7. Generate track from several sequences of splines -

8. Draw splines using recursive subdivision - Y

9. Render environment in a better manner - 

10. Improved coaster normals -

11. Modify velocity with which the camera moves - Y

12. Derive the steps that lead to the physically realistic equation of updating u -

Additional Features: (Please document any additional features you may have implemented other than the ones described above)
1. 
2.
3.

Open-Ended Problems: (Please document approaches to any open-ended problems that you have tackled)
1. I calculated the rails by using a constant offset and modifying the 
 existing spline positions.
2. I used the tangent vectors array to find the magnitude of dp/du.
3. I struggled a bit with counting the number of vertices with the recursive method, but I ended up just running the recursion once prior to the actual function.

Keyboard/Mouse controls: (Please document Keyboard/Mouse controls if any)
1. Press 'x' to start taking screenshots (for animation)
2. Press 'y' to stop taking screenshots
3.

Names of the .cpp files you made changes to:
1. hw2.cpp
2. 
3.

Comments : (If any)
I also added .glsl files for the Phong shaders and the texture shaders!