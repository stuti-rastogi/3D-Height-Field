# Generating Height Fields

-------------------------------------------------------------------------------

Name: Stuti Rastogi

Date: 02/14/18

-------------------------------------------------------------------------------

Description: 

The program intends to read a grayscale JPEG heightmap 
and generate a 3D height field. There is functionality to rotate, 
translate, scale as well zoom in and zoom out using keyboard 
and mouse inputs. The height field can be viewed in a solid fill mode, 
wire mesh mode or points mode. Technology used is C++ and OpenGL (core).

-------------------------------------------------------------------------------

Instructions:

Following are the keys used to interact with the application

f - fill mode (solid is default)

w - wire mode

p - points mode

o - overlay mode (mesh on top of solid fill)

x - screenshot

r - rotate (rotate is set to default)

t - translate

s - scale

+ - zoom in (shift + '=' key or '+' key on numpad)

- - zoom out

ESC - exit

(Note: All keys are lowercase)

Following are the mouse buttons used:

LEFT - x and y transformations

RIGHT - z transformation

-------------------------------------------------------------------------------

Extra Credit:

1. Zoom in and zoom out functionality

2. Overlay wire mesh on top of solid mesh using an offset

-------------------------------------------------------------------------------

Comments:

- There was some issue in the GLUT modifiers on my Mac, I was unable to get ALT 
CTRL to be recognised. Hence made use of keyboard input to change what type of
transormation is being performed

Eg: In order to do translate: first press 't' and release. 
The terminal will display a message saying "You can translate now." 
Translate along X, Y using the Left mouse button and dragging, and along Z by 
dragging right mouse button.

- There are 4 VBOs and VAOs in the application - solid, wire, points and
overlay. The overlay required a separate VAO/VBO since I wanted to render
a different color wire mesh on the solid mesh, as using the same wire VAO did
not look very good. There is a separate color array for Overlay but the same
positions array as wire is used. The wire positions array and overlay color
array are loaded into the VBO.

- The zoom in and zoom out happen in increments of 5 degrees. The minimum FoV
is 3, and maximum is 170. I obtained these by trial of different values. If
the user tries to zoom in/out beyond these, an appropriate message is displayed
and the minimum/maximum FoV is set.

- The idle function takes the screenshots and saves them as 000.jpg, 001.jpg,
and so on. There is a 'stop' flag which is set to 1 as soon as 300 screenshots
have been captured, and after that no more screenshots are taken. This is to 
prevent the program from exiting and it will still continue to run. An
animation video (output.mp4) has been placed in the same directory for the 
submission requirement.

-------------------------------------------------------------------------------

References:

http://prideout.net/archive/colors.php

http://www.glprogramming.com/red/chapter06.html

https://www.khronos.org/registry/OpenGL-Refpages/gl4/

Credits to Prof. Jernej Barbic and the team of CSCI 420 for the starter code.

-------------------------------------------------------------------------------
