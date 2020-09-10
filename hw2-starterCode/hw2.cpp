/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C++ starter code

  Student username: wangrach
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include <glm/gtc/type_ptr.hpp>
#include "imageIO.h"
#include "glutHeader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "openGLHeader.h"
#include "imageIO.h"
#include <iostream>
#include <cstring>
#include <cmath>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 Homework II";

GLuint coasterVertexBuffer, coasterColorVertexBuffer;
GLuint rightRailBuffer, rightRailColorBuffer;
GLuint leftRailBuffer, leftRailColorBuffer;
GLuint coasterVertexArray;
GLuint ground, groundUVs, groundVAO, groundHandle;
GLuint sky, skyUVs, skyVAO, skyHandle;

int numVertices;
int camIndex;
int posIndex;
int numSky;
float timeStep = 20;
float maxHeight;
float gravity = 9.8;

float alpha = 0.05;
float beta = 0.005;
float maxDim = 100.0;
float minDim = -20;

int imageNumber;

GLint h_modelViewMatrix, h_projectionMatrix, h_viewLightDirection, h_normalMatrix, h_textureImage;
OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;

BasicPipelineProgram * texturePipelineProgram;
GLuint program, textureProgram;

glm::vec3 * splinePos;
glm::vec4 * splineCol;

glm::vec3 eye;
glm::vec3 focus;
glm::vec3 up;

glm::vec3 * coasterPos;
glm::vec3 * coasterCol;

glm::vec3 * rightRailPos;
glm::vec3 * rightRailCol;

glm::vec3 * leftRailPos;
glm::vec3 * leftRailCol;

glm::vec3 * groundPos;
glm::vec2 * groundTex;

glm::vec3 * skyPos;
glm::vec2 * skyTex;

glm::vec3 * tangentVectors;
glm::vec3 * normalVectors;
glm::vec3 * binormalVectors;

float s = 0.5;
// catmull-rom spline basis matrix
glm::mat4 crBasis = glm::mat4(-s, 2.0*s, -s, 0.0,
                             2.0-s, s-3.0, 0.0, 1,
                              s-2.0, 3.0-2.0*s, s, 0.0,
                               s, -s, 0.0, 0.0);

// represents one control point along the spline 
struct Point 
{
  double x;
  double y;
  double z;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline 
{
  int numControlPoints;
  Point * points;
};

// the spline array 
Spline * splines;
// total number of splines 
int numSplines;

bool screenshot;

using namespace std;

void setTextureUnit(GLint unit) {
  // texturePipelineProgram->Bind();
  glActiveTexture(unit); // select the active texture unit
  // get a handle to the “textureImage” shader variable
  h_textureImage = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "textureImage");
  // deem the shader variable “textureImage” to read from texture unit “unit”
  glUniform1i(h_textureImage, unit - GL_TEXTURE0);
}

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  int scale = 2;
  int ww = windowWidth * scale;
  int hh = windowHeight * scale;
  unsigned char * screenshotData = new unsigned char[ww * hh * 3];
  glReadPixels(0, 0, ww, hh, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  unsigned char * screenshotData1 = new unsigned char[windowWidth * windowHeight * 3];
  for (int h = 0; h < windowHeight; h++) {
    for (int w = 0; w < windowWidth; w++) {
      int h1 = h * scale;
      int w1 = w * scale;
      screenshotData1[(h * windowWidth + w) * 3] = screenshotData[(h1 * ww + w1) * 3];
      screenshotData1[(h * windowWidth + w) * 3 + 1] = screenshotData[(h1 * ww + w1) * 3 + 1];
      screenshotData1[(h * windowWidth + w) * 3 + 2] = screenshotData[(h1 * ww + w1) * 3 + 2];
    }
  }

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData1);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
  delete [] screenshotData1;
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
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
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
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
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
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

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // take a screenshot
      screenshot = true;
      break;
    case 'y':
    // stop taking screenshots
      screenshot = false;
      break;
    break;

  }
}

int loadSplines(char * argv) 
{
  char * cName = (char *) malloc(128 * sizeof(char));
  FILE * fileList;
  FILE * fileSpline;
  int iType, i = 0, j, iLength;

  // load the track file 
  fileList = fopen(argv, "r");
  if (fileList == NULL) 
  {
    printf ("can't open file\n");
    exit(1);
  }
  
  // stores the number of splines in a global variable 
  fscanf(fileList, "%d", &numSplines);

  splines = (Spline*) malloc(numSplines * sizeof(Spline));

  // reads through the spline files 
  for (j = 0; j < numSplines; j++) 
  {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) 
    {
      printf ("can't open file\n");
      exit(1);
    }

    // gets length for spline file
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    // allocate memory for all the points
    splines[j].points = (Point *)malloc(iLength * sizeof(Point));
    splines[j].numControlPoints = iLength;

    // saves the data to the struct
    while (fscanf(fileSpline, "%lf %lf %lf", 
	   &splines[j].points[i].x, 
	   &splines[j].points[i].y, 
	   &splines[j].points[i].z) != EOF) 
    {
      if (i == 0) {
        maxHeight = splines[j].points[i].y;
      }
      else {
        if (splines[j].points[i].y > maxHeight) {
          maxHeight = splines[j].points[i].y;
        }
      }
      i++;
    }
  }

  free(cName);

  return 0;
}

void countVertices(float u0, float u1, float maxLineLength,
 int& numVertices, glm::mat3x4 controlMatrix) {
  float uMid = (u0 + u1) / 2.0;
  glm::vec4 params0 = glm::vec4(pow(u0, 3), pow(u0, 2), u0, 1.0);
  glm::vec4 params1 = glm::vec4(pow(u1, 3), pow(u1, 2), u1, 1.0);
  glm::vec3 x0 = params0 * crBasis * controlMatrix;
  glm::vec3 x1 = params1 * crBasis * controlMatrix;
  if (glm::length(x1 - x0) > maxLineLength) {
    countVertices(u0, uMid, maxLineLength, numVertices, controlMatrix);
    countVertices(uMid, u1, maxLineLength, numVertices, controlMatrix);
  }
  else {
    numVertices+=2;
  }
}

void sloansMethod() {
  int vecIndex = 0;
  glm::vec3 arbitraryVec = glm::vec3(1, 0, 0);
  normalVectors[vecIndex] = glm::normalize(glm::cross(tangentVectors[vecIndex], arbitraryVec));
  binormalVectors[vecIndex] = glm::normalize(glm::cross(tangentVectors[vecIndex],
    normalVectors[vecIndex]));
  vecIndex++;
  for (int u = 1; u < numVertices; u++) {
      normalVectors[vecIndex] = glm::normalize(glm::cross(binormalVectors[vecIndex-1], tangentVectors[vecIndex]));
      binormalVectors[vecIndex] = glm::normalize(glm::cross(tangentVectors[vecIndex], normalVectors[vecIndex]));
      vecIndex++;
  }
}

// ** remember, pass numVertices and numIndex as a reference! **
void subdivide(float u0, float u1, float maxLineLength, int& index, glm::mat3x4 controlMatrix) {
  float uMid = (u0 + u1) / 2.0;
  glm::vec4 params0 = glm::vec4(pow(u0, 3), pow(u0, 2), u0, 1.0);
  glm::vec4 params1 = glm::vec4(pow(u1, 3), pow(u1, 2), u1, 1.0);
  glm::vec3 x0 = params0 * crBasis * controlMatrix;
  glm::vec3 x1 = params1 * crBasis * controlMatrix;
  glm::vec4 derivative0 = glm::vec4((3*pow(u0, 2)), (2*u0), 1, 0);
  glm::vec4 derivative1 = glm::vec4((3*pow(u1, 2)), (2*u1), 1, 0);
  if (glm::length(x1 - x0) > maxLineLength) {
    subdivide(u0, uMid, maxLineLength, index, controlMatrix);
    subdivide(uMid, u1, maxLineLength, index, controlMatrix);
  }
  else {
    splinePos[index] = x0;
    splineCol[index] = glm::vec4(u0, 1.0, .5, 1.0);
    tangentVectors[index] = glm::normalize(derivative0 * crBasis * controlMatrix);
    index++;

    splinePos[index] = x1;
    splineCol[index] = glm::vec4(u1, 1.0, .5, 1.0);
    tangentVectors[index] = glm::normalize(derivative1 * crBasis * controlMatrix);
    index++;
  }
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
  // read the texture image
  ImageIO img;
  ImageIO::fileFormatType imgFormat;
  ImageIO::errorType err = img.load(imageFilename, &imgFormat);

  if (err != ImageIO::OK) 
  {
    printf("Loading texture from %s failed.\n", imageFilename);
    return -1;
  }

  // check that the number of bytes is a multiple of 4
  if (img.getWidth() * img.getBytesPerPixel() % 4) 
  {
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // allocate space for an array of pixels
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // fill the pixelsRGBA array with the image pixels
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++) 
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // bind the texture
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // initialize the texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // generate the mipmaps for this texture
  glGenerateMipmap(GL_TEXTURE_2D);

  // set the texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // query support for anisotropic texture filtering
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // set anisotropic texture filtering
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // query for any errors
  GLenum errCode = glGetError();
  if (errCode != 0 && errCode != 1281) 
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    cout << gluErrorString(glGetError()) << endl;
    return -1;
  }
  
  // de-allocate the pixel array -- it is no longer needed
  delete [] pixelsRGBA;

  return 0;
}

void initGround() {
  groundPos = (glm::vec3* )malloc(sizeof(glm::vec3) * 6);
  groundTex = (glm::vec2* )malloc(sizeof(glm::vec2) * 6);

  glGenTextures(1, &groundHandle);
  int code = initTexture("images/moss.jpg", groundHandle);
  if (code) {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  glm::vec3 groundOne = glm::vec3(maxDim, minDim, maxDim);
  glm::vec3 groundTwo = glm::vec3(maxDim, minDim, -maxDim);
  glm::vec3 groundThree = glm::vec3(-maxDim, minDim, -maxDim);
  glm::vec3 groundFour = glm::vec3(-maxDim, minDim, maxDim);

  glm::vec2 groundTexOne = glm::vec2(0.0, 0.0);
  glm::vec2 groundTexTwo = glm::vec2(0.0, 1.0);
  glm::vec2 groundTexThree = glm::vec2(1.0, 1.0);
  glm::vec2 groundTexFour = glm::vec2(1.0, 0.0);

  int index = 0;
  // half quad
  groundPos[index] = groundOne;
  groundTex[index] = groundTexOne;
  index++;
  groundPos[index] = groundTwo;
  groundTex[index] = groundTexTwo;
  index++;
  groundPos[index] = groundThree;
  groundTex[index] = groundTexThree;
  index++;

  // full quad
  groundPos[index] = groundThree;
  groundTex[index] = groundTexThree;
  index++;
  groundPos[index] = groundTwo;
  groundTex[index] = groundTexTwo;
  index++;
  groundPos[index] = groundFour;
  groundTex[index] = groundTexFour;

  texturePipelineProgram->Bind();

  glGenBuffers(1, &ground);
  glBindBuffer(GL_ARRAY_BUFFER, ground);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 6, groundPos,
              GL_STATIC_DRAW);

  glGenBuffers(1, &groundUVs);
  glBindBuffer(GL_ARRAY_BUFFER, groundUVs);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 6, groundTex, GL_STATIC_DRAW);

}

void initSky() {
  skyPos = (glm::vec3* )malloc(sizeof(glm::vec3) * 6);
  skyTex = (glm::vec2* )malloc(sizeof(glm::vec2) * 6);

  glGenTextures(1, &skyHandle);
  int code = initTexture("images/Raspberry.jpg", skyHandle);
  if (code) {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  int index = 0;
  glm::vec3 firstSkyOne = glm::vec3(-maxDim, -maxDim, maxDim);
  glm::vec3 firstSkyTwo = glm::vec3(maxDim, -maxDim, maxDim);
  glm::vec3 firstSkyThree = glm::vec3(maxDim, maxDim, maxDim);
  glm::vec3 firstSkyFour = glm::vec3(-maxDim, maxDim, maxDim);;

  glm::vec2 groundTexOne = glm::vec2(0.0, 0.0);
  glm::vec2 groundTexTwo = glm::vec2(0.0, 1.0);
  glm::vec2 groundTexThree = glm::vec2(1.0, 1.0);
  glm::vec2 groundTexFour = glm::vec2(1.0, 0.0);
  // half quad
  skyPos[index] = firstSkyOne;
  skyTex[index] = groundTexOne;
  index++;
  skyPos[index] = firstSkyTwo;
  skyTex[index] = groundTexTwo;
  index++;
  skyPos[index] = firstSkyThree;
  skyTex[index] = groundTexThree;
  index++;

  // full quad
  skyPos[index] = firstSkyThree;
  skyTex[index] = groundTexThree;
  index++;
  skyPos[index] = firstSkyTwo;
  skyTex[index] = groundTexTwo;
  index++;
  skyPos[index] = firstSkyFour;
  skyTex[index] = groundTexFour;

  texturePipelineProgram->Bind();

  glGenBuffers(1, &sky);
  glBindBuffer(GL_ARRAY_BUFFER, sky);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 6, skyPos,
              GL_STATIC_DRAW);

  glGenBuffers(1, &skyUVs);
  glBindBuffer(GL_ARRAY_BUFFER, skyUVs);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 6, skyTex, GL_STATIC_DRAW);

}

void renderSky() {

  texturePipelineProgram->Bind();
  glGenVertexArrays(1, &skyVAO);
  glBindVertexArray(skyVAO);

  glBindBuffer(GL_ARRAY_BUFFER, sky);
  GLuint loc2 =
    glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc2);
  glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, skyUVs);
  GLuint loc3 = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(loc3);
  glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  setTextureUnit(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, skyHandle);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  // glDisableVertexAttribArray(loc2);
  // glDisableVertexAttribArray(loc3);
  glBindVertexArray(0);

}

void renderGround() {

  texturePipelineProgram->Bind();
  glGenVertexArrays(1, &groundVAO);
  glBindVertexArray(groundVAO);

  glBindBuffer(GL_ARRAY_BUFFER, ground);
  GLuint loc0 =
    glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc0);
  glVertexAttribPointer(loc0, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, groundUVs);
  GLuint loc1 = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(loc1);
  glVertexAttribPointer(loc1, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  setTextureUnit(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, groundHandle);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  // glDisableVertexAttribArray(loc0);
  // glDisableVertexAttribArray(loc1);
  glBindVertexArray(0);

}

void environment() {

  int index = 0;
  initGround();
  initSky();
  glBindVertexArray(0);

}

void coasterVBO(glm::vec3 vertex0, glm::vec3 vertex1, glm::vec3 vertex2,
 glm::vec3 vertex3, glm::vec3 vertex4, glm::vec3 vertex5,
  glm::vec3 vertex6, glm::vec3 vertex7, int& index) {
   glm::vec3 color;
   // first quad
   coasterPos[index] = vertex0; color = glm::normalize(vertex0);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex1; color = glm::normalize(vertex1);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex3; color = glm::normalize(vertex3);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex3; color = glm::normalize(vertex3);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex2; color = glm::normalize(vertex2);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex1; color = glm::normalize(vertex1);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;

   // second quad
   coasterPos[index] = vertex0; color = glm::normalize(vertex0);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex1; color = glm::normalize(vertex1);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex4; color = glm::normalize(vertex4);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex4; color = glm::normalize(vertex4);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex1; color = glm::normalize(vertex1);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex5; color = glm::normalize(vertex5);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;

   // third quad
   coasterPos[index] = vertex0; color = glm::normalize(vertex0);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex4; color = glm::normalize(vertex4);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex7; color = glm::normalize(vertex7);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex7; color = glm::normalize(vertex7);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex0; color = glm::normalize(vertex0);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex3; color = glm::normalize(vertex3);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;

   // fourth quad
   coasterPos[index] = vertex3; color = glm::normalize(vertex3);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex2; color = glm::normalize(vertex2);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex7; color = glm::normalize(vertex7);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex7; color = glm::normalize(vertex7);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex2; color = glm::normalize(vertex2);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex6; color = glm::normalize(vertex6);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;

   // fifth quad
   coasterPos[index] = vertex1; color = glm::normalize(vertex1);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex2; color = glm::normalize(vertex2);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex5; color = glm::normalize(vertex5);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex5; color = glm::normalize(vertex5);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex6; color = glm::normalize(vertex6);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex2; color = glm::normalize(vertex2);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;

   // sixth quad
   coasterPos[index] = vertex4; color = glm::normalize(vertex4);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex5; color = glm::normalize(vertex5);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex7; color = glm::normalize(vertex7);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex7; color = glm::normalize(vertex7);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex6; color = glm::normalize(vertex6);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
   coasterPos[index] = vertex5; color = glm::normalize(vertex5);
   coasterCol[index] = glm::vec3(color.x, color.y, color.z); index++;
}

void initCoaster(int& index) {
  coasterPos = (glm::vec3* )malloc(sizeof(glm::vec3) * numVertices * 36 * 3);
  coasterCol = (glm::vec3* )malloc(sizeof(glm::vec3) * numVertices * 36 * 3);
  for (int i = 0; i < numVertices-1; i++) {
    // first face
    glm::vec3 vertex0 = splinePos[i] + (alpha * (-normalVectors[i] + binormalVectors[i]));
    glm::vec3 vertex1 = splinePos[i] + (alpha * (normalVectors[i] + binormalVectors[i]));
    glm::vec3 vertex2 = splinePos[i] + (alpha * (normalVectors[i] - binormalVectors[i]));
    glm::vec3 vertex3 = splinePos[i] + (alpha * (-normalVectors[i] - binormalVectors[i]));
    // second face
    glm::vec3 vertex4 = splinePos[i+1] + (alpha * (-normalVectors[i+1] - binormalVectors[i+1]));
    glm::vec3 vertex5 = splinePos[i+1] + (alpha * (-normalVectors[i+1] - binormalVectors[i+1]));
    glm::vec3 vertex6 = splinePos[i+1] + (alpha * (-normalVectors[i+1] - binormalVectors[i+1]));
    glm::vec3 vertex7 = splinePos[i+1] + (alpha * (-normalVectors[i+1] - binormalVectors[i+1]));
    // color

    coasterVBO(vertex0, vertex1, vertex2, vertex3, vertex4, vertex5, vertex6, vertex7, index);
  }

}

void initRails(int& index) {
  for (int i = 0; i < numVertices-1; i++) {
    glm::vec3 rightRail = splinePos[i] + (glm::vec3(0.0, -.15, 0.0));
    glm::vec3 leftRail = splinePos[i] - (glm::vec3(0.0, -.15, 0.0));
    glm::vec3 rightRailNext = splinePos[i+1] + (glm::vec3(0.0, -.15, 0.0));
    glm::vec3 leftRailNext = splinePos[i+1] - (glm::vec3(0.0, -.15, 0.0));
    // first face
    glm::vec3 vertex0 = rightRail + (beta * (-normalVectors[i] + binormalVectors[i]));
    glm::vec3 vertex1 = rightRail + (beta * (normalVectors[i] + binormalVectors[i]));
    glm::vec3 vertex2 = rightRail + (beta * (normalVectors[i] - binormalVectors[i]));
    glm::vec3 vertex3 = rightRail + (beta * (-normalVectors[i] - binormalVectors[i]));
    // second face
    glm::vec3 vertex4 = rightRailNext + (beta * (-normalVectors[i+1] - binormalVectors[i+1]));
    glm::vec3 vertex5 = rightRailNext + (beta * (-normalVectors[i+1] - binormalVectors[i+1]));
    glm::vec3 vertex6 = rightRailNext + (beta * (-normalVectors[i+1] - binormalVectors[i+1]));
    glm::vec3 vertex7 = rightRailNext + (beta * (-normalVectors[i+1] - binormalVectors[i+1]));
    // color
    coasterVBO(vertex0, vertex1, vertex2, vertex3, vertex4, vertex5, vertex6, vertex7, index);

    vertex0 = leftRail + (beta * (-normalVectors[i] + binormalVectors[i]));
    vertex1 = leftRail + (beta * (normalVectors[i] + binormalVectors[i]));
    vertex2 = leftRail + (beta * (normalVectors[i] - binormalVectors[i]));
    vertex3 = leftRail + (beta * (-normalVectors[i] - binormalVectors[i]));
    // second face
    vertex4 = leftRailNext + (beta * (-normalVectors[i+1] - binormalVectors[i+1]));
    vertex5 = leftRailNext + (beta * (-normalVectors[i+1] - binormalVectors[i+1]));
    vertex6 = leftRailNext + (beta * (-normalVectors[i+1] - binormalVectors[i+1]));
    vertex7 = leftRailNext + (beta * (-normalVectors[i+1] - binormalVectors[i+1]));

    coasterVBO(vertex0, vertex1, vertex2, vertex3, vertex4, vertex5, vertex6, vertex7, index);
  }

  glGenBuffers(1, &coasterVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, coasterVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * numVertices * 36 * 3, coasterPos,
              GL_STATIC_DRAW);

  glGenBuffers(1, &coasterColorVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, coasterColorVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * numVertices * 36 * 3, coasterCol, GL_STATIC_DRAW);

}

void renderCoaster() {
  pipelineProgram->Bind();

  int index = 0;
  initCoaster(index);
  initRails(index);

  glGenVertexArrays(1, &coasterVertexArray);
  glBindVertexArray(coasterVertexArray);

  glBindBuffer(GL_ARRAY_BUFFER, coasterVertexArray);
  GLuint loc =
      glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, coasterColorVertexBuffer);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

}

void initSplines() {
  // leaves for-loop with number of vertices
    for (int i = 0; i < numSplines; i++) {
      for (int j = 0; j < splines[i].numControlPoints-3; j++) {
        for (float k = 0.0; k < 1.0; k+=.0001) {
          Point one = splines[i].points[j];
          Point two = splines[i].points[j+1];
          Point three = splines[i].points[j+2];
          Point four = splines[i].points[j+3];
          glm::mat3x4 controlMatrix = glm::mat3x4(one.x, two.x, three.x, four.x,
                                                  one.y, two.y, three.y, four.y,
                                                  one.z, two.z, three.z, four.z);
          countVertices(k, (k+.0001), .005, numVertices, controlMatrix);
        }
      }
    }

    splinePos = (glm::vec3* )malloc(sizeof(glm::vec3) * numVertices);
    splineCol = (glm::vec4* )malloc(sizeof(glm::vec4) * numVertices);

    tangentVectors = (glm::vec3* )malloc(sizeof(glm::vec3) * numVertices);
    normalVectors = (glm::vec3* )malloc(sizeof(glm::vec3) * numVertices);
    binormalVectors = (glm::vec3* )malloc(sizeof(glm::vec3) * numVertices);
    
    int index = 0;

    // calculating splines with subdivide
    for (int i = 0; i < numSplines; i++) {
      for (int j = 0; j < splines[i].numControlPoints-3; j++) {
        Point one = splines[i].points[j];
        Point two = splines[i].points[j+1];
        Point three = splines[i].points[j+2];
        Point four = splines[i].points[j+3];
        glm::mat3x4 controlMatrix = glm::mat3x4(one.x, two.x, three.x, four.x,
                                                  one.y, two.y, three.y, four.y,
                                                  one.z, two.z, three.z, four.z);                                 
        // subdivide
        for (float k = 0.0; k < 1.0; k+=.0001) {
          subdivide(k, (k+.0001), .005, index, controlMatrix);
        }
      }
    }

    sloansMethod();
}

// helper function to render the entire scene
void render() {
    
    initSplines();
    renderCoaster();
    environment();

}

void initScene(int argc, char *argv[])
{
  numVertices = 0;
  camIndex = 0;
  posIndex = 0;
  imageNumber = 0;

  screenshot = false;
   
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->BuildShadersFromFiles(shaderBasePath, "phong.vertexShader.glsl", "phong.fragmentShader.glsl");
  if (ret != 0) abort();
  texturePipelineProgram = new BasicPipelineProgram;
  ret = texturePipelineProgram->BuildShadersFromFiles(shaderBasePath, "textures.vertexShader.glsl", "textures.fragmentShader.glsl");
  if (ret != 0) abort();

  glEnable(GL_DEPTH_TEST);

  render();

  std::cout << "GL error: " << glGetError() << std::endl;
  cout << gluErrorString(glGetError()) << endl;

}

void transform() {

  pipelineProgram->Bind();

  h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix");
  h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix");
  h_viewLightDirection = glGetUniformLocation(program, "viewLightDirection");
  h_normalMatrix = glGetUniformLocation(program, "normalMatrix");

  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.LookAt(eye.x, eye.y, eye.z, focus.x, focus.y, focus.z, up.x, up.y, up.z);

  // the following line is pseudo-code:
  float tempView[16];
  matrix.GetMatrix(tempView);
  glm::mat4 view = glm::make_mat4(tempView);
  glm::vec4 viewVec = (view * glm::vec4(0.4, 0.4, .8, 0.0));
  float viewLightDirection[3] = { viewVec.x, viewVec.y, viewVec.z }; // light direction in the view space
  // upload viewLightDirection to the GPU
  glUniform3fv(h_viewLightDirection, 1, viewLightDirection);

  // matrix transforms
  matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  // x dimension
  matrix.Rotate(landRotate[0], 1, 0, 0);
  // y dimension
  matrix.Rotate(landRotate[1], 0, 1, 0);
  // z dimesnion
  matrix.Rotate(landRotate[2], 0, 0, 1);
  matrix.Scale(landScale[0], landScale[1], landScale[2]);

  // binding modelview matrix
  float m[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(m);
  glUniformMatrix4fv(h_modelViewMatrix, 1, GL_FALSE, m);

  // binding projection matrix
  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  matrix.Perspective(90.0, 1.0 * windowHeight/windowWidth, .01, 100.0);
  matrix.GetMatrix(p);
  glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, p);

  float n[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetNormalMatrix(n); // get normal matrix
  // upload n to the GPU
  GLboolean isRowMajor = GL_FALSE;
  glUniformMatrix4fv(h_normalMatrix, 1, isRowMajor, n);

  // set variable
  pipelineProgram->SetModelViewMatrix(m);
  pipelineProgram->SetProjectionMatrix(p);

  glBindVertexArray(coasterVertexArray);
  glDrawArrays(GL_TRIANGLES, 0, numVertices * 36 * 3);

  glBindVertexArray(0);

  texturePipelineProgram->Bind();

  h_modelViewMatrix = glGetUniformLocation(textureProgram, "modelViewMatrix");
  h_projectionMatrix = glGetUniformLocation(textureProgram, "projectionMatrix");

  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(m);
  glUniformMatrix4fv(h_modelViewMatrix, 1, GL_FALSE, m);

  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);
  glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, p);

  texturePipelineProgram->SetModelViewMatrix(m);
  texturePipelineProgram->SetProjectionMatrix(p);

  renderGround();
  // renderSky();

}

void setCamera(int cameraIndex) {
  eye = glm::vec3(splinePos[camIndex].x - (binormalVectors[camIndex].x * 0.5),
   splinePos[camIndex].y - (binormalVectors[camIndex].y * 0.5), 
   splinePos[camIndex].z - (binormalVectors[camIndex].z * 0.5));
  focus = glm::vec3(splinePos[camIndex].x + tangentVectors[camIndex].x, splinePos[camIndex].y
   + tangentVectors[camIndex].y, splinePos[camIndex].z + tangentVectors[camIndex].z);
  up = glm::vec3(-binormalVectors[camIndex].x, -binormalVectors[camIndex].y, -binormalVectors[camIndex].z);
}

void displayFunc()
{
  // render some stuff...
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  program = pipelineProgram->GetProgramHandle();
  // textureProgram = texturePipelineProgram->GetProgramHandle();

  setCamera(camIndex);

  // physically realistic camera movement
  camIndex +=
   (int)timeStep*(sqrt(2*gravity*((maxHeight+1)-splinePos[camIndex].y))/glm::length(tangentVectors[camIndex]));

  if (camIndex > numVertices) {
    exit(0);
  }

  transform();

  glutSwapBuffers();
}

void idleFunc()
{
  char imageName[50];
  if (screenshot) {
    sprintf(imageName, "%.3d.jpg", imageNumber);
    imageNumber++;
    saveScreenshot(imageName);
  }
  // make the screen update 
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  matrix.Perspective(54.0f, (float)w / (float)h, 0.01f, 100.0f);
}

int main(int argc, char *argv[])
{
  if (argc<2)
  {  
    printf ("usage: %s <trackfile>\n", argv[0]);
    exit(0);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

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

  // load the splines from the provided filename
  loadSplines(argv[1]);
  initScene(argc, argv);
  

  printf("Loaded %d spline(s).\n", numSplines);
  for(int i=0; i<numSplines; i++)
  printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

  // sink forever into the glut loop
  glutMainLoop();
}


