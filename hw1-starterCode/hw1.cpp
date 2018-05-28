/*
	CSCI 420 Spring 2018
	Assignment 1
	Generating Height Fields

	Name: Stuti Rastogi
	USC ID: 8006687469
	Email: srrastog@usc.edu

	Date: 02/14/18

	Description: The program intends to read a grayscale JPEG heightmap 
	and generate a 3D height field. There is functionality to rotate, 
	translate, scale as well zoom in and zoom out using keyboard 
	and mouse inputs. The height field can be viewed in a solid fill mode, 
	wire mesh mode or points mode. Technology used is C++ and OpenGL (core).
*/

// include headers
#include <iostream>
#include <cstring>
#include "openGLHeader.h"
#include "glutHeader.h"

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"

#ifdef WIN32
	#ifdef _DEBUG
		#pragma comment(lib, "glew32d.lib")
	#else
	    #pragma comment(lib, "glew32.lib")
	#endif
#endif

// directory where the shaders sit
#ifdef WIN32
	char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
	char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

// printing some basic information on the terminal
using namespace std;

int mousePos[2]; 			// x,y coordinate of the mouse position

int leftMouseButton = 0; 	// 1 if pressed, 0 if not 
int middleMouseButton = 0; 	// 1 if pressed, 0 if not
int rightMouseButton = 0; 	// 1 if pressed, 0 if not

// to keep track of the current transformation to be executed
typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;		// default is rotate initially

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

// window properties
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

// variables for reading height map image
ImageIO * heightmapImage;
GLuint imageHeight;
GLuint imageWidth;

// pipeline program and matrix variables
OpenGLMatrix * matrix;
BasicPipelineProgram * pipelineProgram;
GLuint programHandler;

// VBOs and VAOs for the different solid, wire, point and overlay modes
GLuint vaoSolid;
GLuint vboSolid;
GLuint vaoWire;
GLuint vboWire;
GLuint vaoPoint;
GLuint vboPoint;
GLuint vaoOverlay;
GLuint vboOverlay;

// variables used to fill the position and color arrays for 
// solid and points modes
GLuint numVerticesSolid;
GLuint numQuadsSolid;
GLuint sizeOfPositionsSolid;		// size of positions array
GLuint sizeOfColorsSolid;			// size of colors array
float *vertexPositionsSolid;
float *vertexColorsSolid;

// variables used to fill the position and color arrays for wire mode and overlay mode
GLuint numVerticesWire;
GLuint numQuadsWire;
GLuint sizeOfPositionsWire;			// size of positions array
GLuint sizeOfColorsWire;			// size of colors array
float *vertexPositionsWire;
float *vertexColorsWire;
float *vertexColorsOverlay;

// variable for the height value fetched from the image loaded for every pixel
GLfloat height;

// indices used in the loops to fill the arrays
GLuint i, j;
GLuint positionArrayIndex;
GLuint colorArrayIndex;
GLuint overlayArrayIndex;

// variables needed in drawArrays call
GLint first;
GLsizei countVertices;

// the variable indicating current mode as solid, wire or points
// 0 - solid (default)
// 1 - wire
// 2 - points
// 3 - overlay (wire mesh on solid)
GLuint mode = 0;

// variables for setting the perspective projection matrix
GLfloat FoV = 45;
GLfloat aspectRatio;

// the matrices used to fetch projection and model view matrices
float p[16];
float m[16];

// variables needed for mapping shader variables to VBO
GLuint loc;
const void * offset;
GLsizei stride;
GLboolean normalized;

// different scaling variables to get visually good results
GLfloat heightScale = 0.25;
GLfloat colorScaleSolid = 5.0;
GLfloat colorScaleWire = 2.0;

// variables used in idle function to create animation
int screenshotCount = 0;
int stop = 0;

// // for milestone
// GLuint vaoID;
// GLuint bufferID;
// float positions [3][3] = {
//   {0.0,0.0,-1.0}, {1.0,0.0,-1.0}, {0.0,1.0,-1.0}
// };
// float colors [3][4] = {
//   {1.0,0.0,0.0,0.0}, {0.0,1.0,0.0,0.0}, {0.0,0.0,1.0,0.0}
// };

// this method fills the positions and colors arrays for the 
// solid and points modes
// we read the height value for every pixel and generate a corresponding 
// vertex to create triangles in our mesh
void fillPositionColorValuesSolid()
{
	numQuadsSolid = (imageWidth-1) * (imageHeight-1);

	// each quad has 2 triangles, each triangle has 3 vertices, hence * 6
	numVerticesSolid = 6 * numQuadsSolid;			
									
	// position needs 3 floats per vertex - x, y, z
	sizeOfPositionsSolid = sizeof(float) * 3 * numVerticesSolid;

	// color needs 4 floats per vertex - r, g, b, a
	sizeOfColorsSolid = sizeof(float) * 4 * numVerticesSolid;

	vertexPositionsSolid = (float *)malloc(sizeOfPositionsSolid);
	vertexColorsSolid = (float *)malloc(sizeOfColorsSolid);

	// run loops for number of quads
	positionArrayIndex = 0;
	colorArrayIndex = 0;

	// for every pixel (x, y):
	// (x, y), (x+1, y) and (x, y+1) form one triangle
	// (x, y+1), (x+1, y) and (x+1, y+1) form one triangle
	// x on the image is x for the vertex
	// y on the image is -z for the vertex
	// color value (height) is y for the vertex
	// color (r,g,b) for the vertex is proportional to the height
	// a is 1 for the vertex
	// increment positionArrayIndex by 3, since 3 values per vertex
	// increment colorArrayIndex by 4, since 4 values per vertex
	for (int y = 0; y < imageHeight-1; y++)
	{
		for (int x = 0; x < imageWidth-1; x++)
		{
			// Vertex 1
			i = x; j = y;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsSolid + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsSolid + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsSolid + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsSolid + colorArrayIndex + 0) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 1) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 2) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;

			// Vertex 2
			i = x+1; j = y;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsSolid + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsSolid + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsSolid + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsSolid + colorArrayIndex + 0) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 1) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 2) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;

			// Vertex 3
			i = x; j = y+1;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsSolid + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsSolid + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsSolid + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsSolid + colorArrayIndex + 0) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 1) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 2) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;

			// Vertex 4
			i = x; j = y+1;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsSolid + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsSolid + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsSolid + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsSolid + colorArrayIndex + 0) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 1) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 2) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;

			// Vertex 5
			i = x+1; j = y;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsSolid + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsSolid + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsSolid + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsSolid + colorArrayIndex + 0) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 1) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 2) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;

			// Vertex 6
			i = x+1; j = y+1;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsSolid + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsSolid + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsSolid + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsSolid + colorArrayIndex + 0) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 1) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 2) = 
												colorScaleSolid * height/255.0;
			*(vertexColorsSolid + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;
		}
	}
}

// this method fills the positions and colors arrays for the wire mode
// as well as the overlay mode
// we read the height value for every pixel and generate a corresponding 
// vertex to create triangles in our mesh
void fillPositionColorValuesWire()
{
	numQuadsWire = (imageWidth-1) * (imageHeight-1);

	// each quad has 5 line segments & 2 vertices per line, hence * 10
	numVerticesWire = 10 * numQuadsWire;

	// position needs 3 floats per vertex - x, y, z
	sizeOfPositionsWire = sizeof(float) * 3 * numVerticesWire;

	// color needs 4 floats per vertex - r, g, b, a
	sizeOfColorsWire = sizeof(float) * 4 * numVerticesWire;

	vertexPositionsWire = (float *)malloc(sizeOfPositionsWire);
	vertexColorsWire = (float *)malloc(sizeOfColorsWire);
	vertexColorsOverlay = (float *)malloc(sizeOfColorsWire);

	// run loops for number of quads
	positionArrayIndex = 0;
	colorArrayIndex = 0;
	overlayArrayIndex = 0;

	// for every pixel (x, y):
	// (x, y), (x+1, y) form one line
	// (x+1, y), (x+1, y+1) form one line
	// (x+1, y+1), (x, y+1) form one line
	// (x, y+1), (x, y+1) form one line
	// (x+1, y), (x, y) form one line
	// x on the image is x for the vertex
	// y on the image is -z for the vertex
	// color value (height) is y for the vertex
	// color (r,g,b) for the vertex is proportional to the height
	// a is 1 for the vertex
	// increment positionArrayIndex by 3, since 3 values per vertex
	// increment colorArrayIndex by 4, since 4 values per vertex
	for (int y = 0; y < imageHeight-1; y++)
	{
		for (int x = 0; x < imageWidth-1; x++)
		{
			// Vertex 1
			i = x; j = y;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsWire + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsWire + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsWire + positionArrayIndex + 2) = 
															(float)(-1.0 * j);

			*(vertexColorsWire + colorArrayIndex + 0) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 1) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 2) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 3) = 1.0;

			*(vertexColorsOverlay + colorArrayIndex + 0) = 4.0 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 1) = 2.88 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 2) = 2.92 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;
			overlayArrayIndex += 4;

			// Vertex 2
			i = x+1; j = y;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsWire + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsWire + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsWire + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsWire + colorArrayIndex + 0) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 1) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 2) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 3) = 1.0;

			*(vertexColorsOverlay + colorArrayIndex + 0) = 4.0 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 1) = 2.88 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 2) = 2.92 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;
			overlayArrayIndex += 4;

			// Vertex 3
			i = x+1; j = y;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsWire + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsWire + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsWire + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsWire + colorArrayIndex + 0) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 1) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 2) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 3) = 1.0;

			*(vertexColorsOverlay + colorArrayIndex + 0) = 4.0 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 1) = 2.88 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 2) = 2.92 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;
			overlayArrayIndex += 4;

			// Vertex 4
			i = x+1; j = y+1;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsWire + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsWire + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsWire + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsWire + colorArrayIndex + 0) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 1) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 2) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 3) = 1.0;

			*(vertexColorsOverlay + colorArrayIndex + 0) = 4.0 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 1) = 2.88 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 2) = 2.92 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;
			overlayArrayIndex += 4;

			// Vertex 5
			i = x+1; j = y+1;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsWire + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsWire + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsWire + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsWire + colorArrayIndex + 0) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 1) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 2) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 3) = 1.0;

			*(vertexColorsOverlay + colorArrayIndex + 0) = 4.0 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 1) = 2.88 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 2) = 2.92 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;
			overlayArrayIndex += 4;
			
			// Vertex 6
			i = x; j = y+1;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsWire + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsWire + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsWire + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsWire + colorArrayIndex + 0) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 1) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 2) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 3) = 1.0;

			*(vertexColorsOverlay + colorArrayIndex + 0) = 4.0 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 1) = 2.88 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 2) = 2.92 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;
			overlayArrayIndex += 4;

			// Vertex 7
			i = x; j = y+1;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsWire + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsWire + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsWire + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsWire + colorArrayIndex + 0) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 1) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 2) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 3) = 1.0;

			*(vertexColorsOverlay + colorArrayIndex + 0) = 4.0 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 1) = 2.88 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 2) = 2.92 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;
			overlayArrayIndex += 4;

			// Vertex 8
			i = x; j = y;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsWire + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsWire + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsWire + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsWire + colorArrayIndex + 0) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 1) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 2) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 3) = 1.0;

			*(vertexColorsOverlay + colorArrayIndex + 0) = 4.0 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 1) = 2.88 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 2) = 2.92 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;
			overlayArrayIndex += 4;

			// Vertex 9
			i = x+1; j = y;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsWire + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsWire + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsWire + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsWire + colorArrayIndex + 0) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 1) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 2) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 3) = 1.0;

			*(vertexColorsOverlay + colorArrayIndex + 0) = 4.0 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 1) = 2.88 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 2) = 2.92 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;
			overlayArrayIndex += 4;

			// Vertex 10
			i = x; j = y+1;
			height = heightScale * heightmapImage->getPixel(i,j,0);
			*(vertexPositionsWire + positionArrayIndex + 0) = 
															(float)i;
			*(vertexPositionsWire + positionArrayIndex + 1) = 
															height;
			*(vertexPositionsWire + positionArrayIndex + 2) = 
															(float)(-1.0 * j);
			*(vertexColorsWire + colorArrayIndex + 0) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 1) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 2) = 
												colorScaleWire * height/255.0;
			*(vertexColorsWire + colorArrayIndex + 3) = 1.0;

			*(vertexColorsOverlay + colorArrayIndex + 0) = 4.0 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 1) = 2.88 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 2) = 2.92 * height/255.0;
			*(vertexColorsOverlay + colorArrayIndex + 3) = 1.0;
			positionArrayIndex += 3;
			colorArrayIndex += 4;
			overlayArrayIndex += 4;
		}
	}
}

// this method writes a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
	unsigned char * screenshotData = 
							new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, 
					GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
	cout << "File " << filename << " saved successfully." << endl;
	else cout << "Failed to save file " << filename << '.' << endl;

	delete [] screenshotData;
}

// this is the method for display callback
void displayFunc()
{
	// every frame clear the color buffer and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// slide 06-12
	// changing matrix mode to Model View Matrix to set up camera
	matrix->SetMatrixMode(OpenGLMatrix::ModelView);
	matrix->LoadIdentity();

	//for milestone
	// Float or double?
	// GLfloat zStudent = 3.0 + 8006687469.0/10000000000.0;
	// cout << zStudent << endl;
	// matrix->LookAt(0,0,zStudent, 0,0,-1, 0,1,0);

	// glBindVertexArray(vaoID); // bind the VAO
	// GLint first = 0;
	// GLsizei count = 3;
	// glDrawArrays(GL_TRIANGLES, first, count);
	// glBindVertexArray(0); // unbind the VAO

	// placing the camera at a height of 300 (Y) towards negative X and 
	// positive Z since image is drawn towards positive X and negative Z
	// lookat is origin to be able to see the image beyond that from 
	// a diagonal 
	matrix->LookAt(
					(imageWidth/-1.0), 300.0, (imageHeight/1.0),
					0.0, 0.0, 0.0,
					0.0, 1.0, 0.0
				);

	// translate is last transformation, hence first here
	matrix->Translate(landTranslate[0], landTranslate[1], landTranslate[2]);

	// rotations about X, Y and Z axes
	matrix->Rotate(landRotate[0], 1.0, 0.0, 0.0);
	matrix->Rotate(landRotate[1], 0.0, 1.0, 0.0);
	matrix->Rotate(landRotate[2], 0.0, 0.0, 1.0);

	// Scale transformation
	matrix->Scale(landScale[0], landScale[1], landScale[2]);

	// this is a translate to have transformations about center of image and
	// not the origin to make it look more natural
	matrix->Translate((imageWidth/-2.0), 0.0f, (imageHeight/2.0));

	// fetching the model view matrix into m and setting it
	matrix->GetMatrix(m);
	pipelineProgram->SetModelViewMatrix(m);

	// based on the value of mode (0, 1 or 2) we decide which VAO to render
	switch (mode)
	{
		// solid
		case 0:
			glBindVertexArray(vaoSolid); 			// bind the VAO
			first = 0;
			countVertices = numVerticesSolid;     	// number of vertices
			glDrawArrays(GL_TRIANGLES, first, countVertices);
			glBindVertexArray(0); 					// unbind the VAO
		break;

		// wire
		case 1:
			glBindVertexArray(vaoWire); 			// bind the VAO
			first = 0;
			countVertices = numVerticesWire;     	// number of vertices
			glDrawArrays(GL_LINES, first, countVertices);
			glBindVertexArray(0); 					// unbind the VAO
		break;

		// points
		case 2:
			glBindVertexArray(vaoPoint); 			// bind the VAO
			first = 0;
			countVertices = numVerticesSolid;     	// number of vertices
			glDrawArrays(GL_POINTS, first, countVertices);
			glBindVertexArray(0); 					// unbind the VAO
		break;

		case 3:
			// glBindVertexArray(vaoSolid); 			// bind the VAO
			// first = 0;
			// countVertices = numVerticesSolid;     	// number of vertices
			// glDrawArrays(GL_TRIANGLES, first, countVertices);
			// glBindVertexArray(0); 					// unbind the VAO

			// glEnable(GL_POLYGON_OFFSET_FILL);		// enable wire offset
			// glPolygonOffset(0.0, -10.0);
			// glBindVertexArray(vaoWire); 			// bind the VAO
			// first = 0;
			// countVertices = numVerticesWire;     	// number of vertices
			// glDrawArrays(GL_LINES, first, countVertices);
			// glBindVertexArray(0); 					// unbind the VAO
			// glDisable(GL_POLYGON_OFFSET_FILL);		// disable wire offset

			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(0.0, 1.0);
			glBindVertexArray(vaoSolid); 			// bind the VAO
			first = 0;
			countVertices = numVerticesSolid;     	// number of vertices
			glDrawArrays(GL_TRIANGLES, first, countVertices);
			glBindVertexArray(0);
			glDisable(GL_POLYGON_OFFSET_FILL);

			glBindVertexArray(vaoOverlay); 			// bind the VAO
			first = 0;
			countVertices = numVerticesWire;     	// number of vertices
			glDrawArrays(GL_LINES, first, countVertices);
			glBindVertexArray(0); 					// unbind the VAO
		break;

		default:
			glBindVertexArray(vaoSolid); 			// bind the VAO
			first = 0;
			countVertices = numVerticesSolid;     	// number of vertices
			glDrawArrays(GL_TRIANGLES, first, countVertices);
			glBindVertexArray(0); 					// unbind the VAO
	}

	glutSwapBuffers();
}

// this is the method for idle callback
// we take repeated screenshots here to create the animation
void idleFunc()
{
	// take screenshots while count is less than 300
	if (stop == 0)
	{
		char saveName [10] = "000.jpg";

		// logic to generate names of the continuous screenshots
		saveName[2] = 48 + (screenshotCount % 10);
		saveName[1] = 48 + ((screenshotCount / 10) % 10);
		saveName[0] = 48 + (screenshotCount / 100);
		saveScreenshot(saveName);
		screenshotCount++;
	}

	// if 300 screenshots taken, still run but just stop taking screenshots
	if (screenshotCount > 300)
	{
		stop = 1;
	}
  	
  	// make the screen update 
	glutPostRedisplay();
}

// this is the method for reshape callback
// set the perspective projection matrix here
void reshapeFunc(int w, int h)
{
	glViewport(0, 0, w, h);

	aspectRatio = (GLfloat)w / (GLfloat) h;       // do not hard code
	matrix->SetMatrixMode(OpenGLMatrix::Projection);
	matrix->LoadIdentity();

	// chose Znear and Zfar to be able to show images upto 768x768
	matrix->Perspective(FoV, aspectRatio, 0.01, 5000.0);

	//upload to GPU
	// NOTE TO SELF: You are getting the current matrix acc to mode
	// loaded in this p that you declared
	// You need this to pass to the BasicPipelineProgram methods
	matrix->GetMatrix(p);     
	pipelineProgram->SetProjectionMatrix(p);

	//reset mode back
	matrix->SetMatrixMode(OpenGLMatrix::ModelView);
}

// this method deals with the dragging of the mouse
// it sets the delta values to be used for different transformations
void mouseMotionDragFunc(int x, int y)
{
	// mouse has moved and one of the mouse buttons is pressed (dragging)
	// the change in mouse position since the last invocation of this function
	int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

	// it sets the transformation values depending on whether we are
	// translating, rotating or scaling
	switch (controlState)
	{
		// translate the landscape
		case TRANSLATE:
			if (leftMouseButton)
			{
				// control x,y translation via the left mouse button
				landTranslate[0] += mousePosDelta[0] * 1.0f;
				landTranslate[1] -= mousePosDelta[1] * 1.0f;
			}
			if (rightMouseButton)
			{
				// control z translation via the right mouse button
				landTranslate[2] += mousePosDelta[1] * 1.0f;
			}
		break;

		// rotate the landscape
		case ROTATE:
			if (leftMouseButton)
			{
				// control x,y rotation via the left mouse button
				landRotate[0] += mousePosDelta[1];
				landRotate[1] += mousePosDelta[0];
			}
			if (rightMouseButton)
			{
				// control z rotation via the right mouse button
				landRotate[2] += mousePosDelta[1];
			}
		break;

		// scale the landscape
		case SCALE:
			if (leftMouseButton)
			{
				// control x,y scaling via the left mouse button
				landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
				landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
			}
			if (rightMouseButton)
			{
				// control z scaling via the right mouse button
				landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
			}
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

// this method is for the mouse motion callback
// store the new mouse position
void mouseMotionFunc(int x, int y)
{
	// mouse has moved
	mousePos[0] = x;
	mousePos[1] = y;
}

// this is the method for the mouse button press callback
// set whether left, right or middle button has been pressed
// using GLUT functions
void mouseButtonFunc(int button, int state, int x, int y)
{
	// a mouse button has has been pressed or depressed

	// keep track of the mouse button state, in leftMouseButton, 
	// middleMouseButton or rightMouseButton variables
	switch (button)
	{
		case GLUT_LEFT_BUTTON:
			leftMouseButton = (state == GLUT_DOWN);
		break;

		case GLUT_MIDDLE_BUTTON:
			middleMouseButton = (state == GLUT_DOWN);
		break;

		case GLUT_RIGHT_BUTTON:
			rightMouseButton = (state == GLUT_DOWN);
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

// this is the method for keyboard callback
// using the keys pressed we can change the modes and the transformations
void keyboardFunc(unsigned char key, int x, int y)
{
	switch (key)
	{
	  	// exit on ESC key, free all buffers
	    case 27: // ESC key
			free(vertexPositionsSolid);
			free(vertexColorsSolid);
			free(vertexPositionsWire);
			free(vertexColorsWire);
			exit(0); // exit the program
	    break;

	    // change mode to point
	    case 'p':
			cout << "Points mode set" << endl;
			mode = 2;
	    break;

	    // change mode to solid
	    case 'f':
			cout << "Solid mode set" << endl;
			mode = 0;
	    break;

	    // change mode to wire
	    case 'w':
			cout << "Wire mode set" << endl;
			mode = 1;
	    break;

	    // change mode to combined (solid + wire)
	    case 'o':
			cout << "Combined" << endl;
			mode = 3;
	    break;

	    // change transformation to translate
	    case 't':
			cout << "You can translate now." << endl;
			controlState = TRANSLATE;
	    break;

	    // change transformation to rotate
	    case 'r':
	    	cout << "You can rotate now." << endl;
	    	controlState = ROTATE;
	    break;

	    // change transformation to scale
	    case 's':
	        cout << "You can scale now." << endl;
	    	controlState = SCALE;
	    break;

	    // zoom in with +
	    case '+':
	        cout << "Zooming in by 5 degrees" << endl;
	    	FoV -= 5;			// reducw the field of view to zoom in
	    	if (FoV < 3)
	    	{
	    		FoV = 3;
	    		cout << "Can't zoomin beyond this." << endl;
	    	}
	    	  
	    	// since FoV changed, set projection matrix again
			matrix->SetMatrixMode(OpenGLMatrix::Projection);
			matrix->LoadIdentity();
			matrix->Perspective(FoV, aspectRatio, 0.01, 5000.0);

			matrix->GetMatrix(p);     
			pipelineProgram->SetProjectionMatrix(p);

			//reset mode back
			matrix->SetMatrixMode(OpenGLMatrix::ModelView);
	    break;

	    case '-':
	        cout << "Zooming out by 5 degrees" << endl;
	    	FoV += 5;			// increase the field of view to zoom out
	    	if (FoV > 170)
	    	{
	    		FoV = 170;
	    		cout << "Can't zoom out beyond this." << endl;
	    	}

			// since FoV changed, set projection matrix again
			matrix->SetMatrixMode(OpenGLMatrix::Projection);
			matrix->LoadIdentity();
			matrix->Perspective(FoV, aspectRatio, 0.01, 5000.0);

			matrix->GetMatrix(p);     
			pipelineProgram->SetProjectionMatrix(p);

			//reset mode back
			matrix->SetMatrixMode(OpenGLMatrix::ModelView);
	    break;

	    case 'x':
	      // take a screenshot
	      saveScreenshot("screenshot.jpg");
	    break;
  	}
}

// this method initialises the pipeline program and sets the program handle
void initPipeline()
{
	pipelineProgram = new BasicPipelineProgram();
	pipelineProgram->Init(shaderBasePath);
	pipelineProgram->Bind();
	programHandler = pipelineProgram->GetProgramHandle();
}

// this method initialises the VBOs, VAOs and binds shader variables
void initBuffers()
{
	//MILESTONE

	// //VAO
	// glGenVertexArrays(1, &vaoID);
	// glBindVertexArray(vaoID);

	// //VBO
	// glGenBuffers(1, &bufferID);
	// glBindBuffer(GL_ARRAY_BUFFER, bufferID);

	// glBufferData(GL_ARRAY_BUFFER, sizeof(positions)+sizeof(colors), 
	//				NULL, GL_STATIC_DRAW);
	// glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(positions), positions);
	// glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions), 
	//				sizeof(colors), colors);

	// //slide 07-32
	// // NOTE TO SELF: Tried putting all this in bindProgram() in
	// // display function - did not work. Use as shown in
	// // init part in Shaders ppt
	// glBindBuffer(GL_ARRAY_BUFFER, bufferID);
	// GLuint loc = glGetAttribLocation(programHandler, "position");
	// glEnableVertexAttribArray(loc);
	// const void * offset = (const void*) 0; 
	// GLsizei stride = 0;
	// GLboolean normalized = GL_FALSE;
	// glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	// loc = glGetAttribLocation(programHandler, "color");
	// glEnableVertexAttribArray(loc);
	// offset = (const void*) sizeof(positions);
	// stride = 0;
	// normalized = GL_FALSE;
	// glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);

	//VBO Solid
	glGenBuffers(1, &vboSolid);
	glBindBuffer(GL_ARRAY_BUFFER, vboSolid);
	glBufferData(GL_ARRAY_BUFFER, sizeOfPositionsSolid+sizeOfColorsSolid, 
					NULL, GL_STATIC_DRAW);
	// fill positions
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeOfPositionsSolid, 
					vertexPositionsSolid);
	// fill colors
	glBufferSubData(GL_ARRAY_BUFFER, sizeOfPositionsSolid, 
					sizeOfColorsSolid, vertexColorsSolid);

	//VBO Wire
	glGenBuffers(1, &vboWire);
	glBindBuffer(GL_ARRAY_BUFFER, vboWire);
	glBufferData(GL_ARRAY_BUFFER, sizeOfPositionsWire+sizeOfColorsWire, 
					NULL, GL_STATIC_DRAW);
	// fill positions
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeOfPositionsWire, 
					vertexPositionsWire);
	// fill colors
	glBufferSubData(GL_ARRAY_BUFFER, sizeOfPositionsWire, 
					sizeOfColorsWire, vertexColorsWire);

	//VBO Point
	glGenBuffers(1, &vboPoint);
	glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
	glBufferData(GL_ARRAY_BUFFER, sizeOfPositionsSolid+sizeOfColorsSolid, 
					NULL, GL_STATIC_DRAW);
	// fill positions
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeOfPositionsSolid, 
					vertexPositionsSolid);
	// fill colors
	glBufferSubData(GL_ARRAY_BUFFER, sizeOfPositionsSolid, 
					sizeOfColorsSolid, vertexColorsSolid);

	//VBO Overlay
	glGenBuffers(1, &vboOverlay);
	glBindBuffer(GL_ARRAY_BUFFER, vboOverlay);
	glBufferData(GL_ARRAY_BUFFER, sizeOfPositionsWire+sizeOfColorsWire, 
					NULL, GL_STATIC_DRAW);
	// fill positions
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeOfPositionsWire, 
					vertexPositionsWire);
	// fill colors
	glBufferSubData(GL_ARRAY_BUFFER, sizeOfPositionsWire, 
					sizeOfColorsWire, vertexColorsOverlay);

	//VAO - Solid
	glGenVertexArrays(1, &vaoSolid);
	glBindVertexArray(vaoSolid);

	//VAO - Wire
	glGenVertexArrays(1, &vaoWire);
	glBindVertexArray(vaoWire);

	//VAO - Points
	glGenVertexArrays(1, &vaoPoint);
	glBindVertexArray(vaoPoint);

	//VAO - Overlay
	glGenVertexArrays(1, &vaoOverlay);
	glBindVertexArray(vaoOverlay);

	// bind buffers for VAO Solid
	glBindBuffer(GL_ARRAY_BUFFER, vboSolid);
	glBindVertexArray(vaoSolid);
	loc = glGetAttribLocation(programHandler, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*) 0; 
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	loc = glGetAttribLocation(programHandler, "color");
	glEnableVertexAttribArray(loc);
	offset = (const void*) (sizeOfPositionsSolid);
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0);       // unbind

	// bind buffers for VAO Wire
	glBindBuffer(GL_ARRAY_BUFFER, vboWire);
	glBindVertexArray(vaoWire);
	loc = glGetAttribLocation(programHandler, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*) 0; 
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	loc = glGetAttribLocation(programHandler, "color");
	glEnableVertexAttribArray(loc);
	offset = (const void*) (sizeOfPositionsWire);
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0);       // unbind

	// bind buffers for VAO Points
	glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
	glBindVertexArray(vaoPoint);
	loc = glGetAttribLocation(programHandler, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*) 0; 
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	loc = glGetAttribLocation(programHandler, "color");
	glEnableVertexAttribArray(loc);
	offset = (const void*) (sizeOfPositionsSolid);
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0);       // unbind

	// bind buffers for VAO Overlay
	glBindBuffer(GL_ARRAY_BUFFER, vboOverlay);
	glBindVertexArray(vaoOverlay);
	loc = glGetAttribLocation(programHandler, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*) 0; 
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	loc = glGetAttribLocation(programHandler, "color");
	glEnableVertexAttribArray(loc);
	offset = (const void*) (sizeOfPositionsWire);
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0);       // unbind
}

// this method takes care of all the initialisations
void initScene(int argc, char *argv[])
{
	// load the image from a jpeg disk file to main memory
	heightmapImage = new ImageIO();
	if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
	{
		cout << "Error reading image " << argv[1] << "." << endl;
		exit(EXIT_FAILURE);
	}

	// store the image height and width to use to fill arrays
	imageWidth = heightmapImage->getWidth();
	imageHeight = heightmapImage->getHeight();

	// set background color
	glClearColor(0.529, 0.808, 0.980, 1.0);

	// fill the position and color arrays for all modes
	fillPositionColorValuesSolid();
	fillPositionColorValuesWire();

	glEnable(GL_DEPTH_TEST);
	matrix = new OpenGLMatrix();
	initPipeline();
	initBuffers();
}

// main method
int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		cout << "The arguments are incorrect." << endl;
		cout << "usage: ./hw1 <heightmap file>" << endl;
		exit(EXIT_FAILURE);
	}

	cout << "Initializing GLUT..." << endl;
	glutInit(&argc,argv);

	cout << "Initializing OpenGL..." << endl;

	#ifdef __APPLE__
		glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | 
							GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
	#else
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | 
							GLUT_STENCIL);
	#endif

	// initialise the window
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(0, 0);  
	glutCreateWindow(windowTitle);

	cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
	cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Shading Language Version: " << 
				glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// tells glut to use a particular display function to redraw 
	glutDisplayFunc(displayFunc);
	// perform animation inside idleFunc
	glutIdleFunc(idleFunc);
	// callback for mouse drags
	glutMotionFunc(mouseMotionDragFunc);
	// callback for idle mouse movement
	glutPassiveMotionFunc(mouseMotionFunc);
	// callback for mouse button changes
	glutMouseFunc(mouseButtonFunc);
	// callback for resizing the window
	glutReshapeFunc(reshapeFunc);
	// callback for pressing the keys on the keyboard
	glutKeyboardFunc(keyboardFunc);

	// init glew
	#ifdef __APPLE__
		// nothing is needed on Apple
	#else
		// Windows, Linux
		GLint result = glewInit();
		if (result != GLEW_OK)
		{
			cout << "error: " << glewGetErrorString(result) << endl;
			exit(EXIT_FAILURE);
		}
	#endif

	// do initialization
	initScene(argc, argv);

	// sink forever into the glut loop
	glutMainLoop();
}