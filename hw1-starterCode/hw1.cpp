/*
    CSCI 420 Computer Graphics, USC
    Assignment 1: Height Fields with Shaders.
    C++ starter code
     
    Student username: lyup
*/

#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include "openGLHeader.h"
#include "glutHeader.h"

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"

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

using namespace std;

int mousePos[2];                                // x,y coordinate of the mouse position

int leftMouseButton = 0;                        // 1 if pressed, 0 if not 
int middleMouseButton = 0;                      // 1 if pressed, 0 if not
int rightMouseButton = 0;                       // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

typedef enum { POINT_MODE, LINE_MODE, TRIANGLE_MODE, SMOOTH_MODE } RENDER_MODE;
RENDER_MODE renderMode = POINT_MODE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";
float p[16];
float m[16];
typedef enum {RED = 0, GREEN, BLUE}; 

OpenGLMatrix* matrix;
BasicPipelineProgram* pipelineProgram;
///////////////////// loading 256* 256 color image /////////////////////////
// color file
ImageIO* colorImage = new ImageIO();;
GLuint color_pix;
GLuint color_height;
GLuint color_width;
bool load_ColorImage() {
    const char* color_file = "./heightmap/grund.jpg";
    colorImage->loadJPEG(color_file);
    color_pix = colorImage->getBytesPerPixel();
    color_height = colorImage->getHeight();
    color_width = colorImage->getWidth();
    return true;
}

///////////////////// loading 256*256 heightfleid image ////////////////////////
ImageIO * heightmapImage;
GLuint image_row, image_col; 
GLuint bytesPerPixel;
GLuint total_vertex;
GLfloat* color;
GLfloat *position;
GLfloat *center;
void load_heightfleid() {

    float height;
    int number = 0;
    float sizeX = 4.0f;
    float sizeY = 4.0f;
    float centerX = 1.0f;
    float centerY = 1.0f;

    image_row = heightmapImage->getHeight();
    image_col = heightmapImage->getWidth();
    bytesPerPixel = heightmapImage->getBytesPerPixel();

    total_vertex = image_row * image_col;

    // position 3
    position = (GLfloat*)malloc(sizeof(GLfloat) * 3 * total_vertex); //(x,y,z) for each vertice
    center = (GLfloat*)malloc(sizeof(GLfloat) * 3 * total_vertex);
    // color 4
    color = (GLfloat*)malloc(sizeof(GLfloat) * 4 * total_vertex); // RGBA for each vertice

    int channel = 0;
    if (bytesPerPixel == 3) {
        channel = GREEN;
    }

    bool color_flag = load_ColorImage(); 
    for (int i = 0; i < image_row; ++i) {
        for (int j = 0; j < image_col; ++j) {
            height = (float)heightmapImage->getPixel(i, j, channel) / 255.0f;

            ///////////////////////// center ////////////////////////////////////////
            GLfloat down_left[3] = { i, height, -j };
            GLfloat top_left[3] = { i,height,-(j + 1) };
            GLfloat down_right[3] = { i+1,height,-j};
            GLfloat top_right[3] = { i+1,height,-(j + 1) };
            for (int i = 0; i < 3; i++) {
                center[i] = (down_left[i] + down_right[i] + top_left[i] + top_right[i]) / 4;
            }
            ////////////////////////////////////////////////////////////////////////////
            //center the heightmap at (0,0), range(-2,2) on X axis and Y axis
            position[number * 3 + 0] = -sizeX / 2.0f + (float)sizeX * i / (image_row - 1);
            position[number * 3 + 1] = -sizeY / 2.0f + (float)sizeY * j / (image_col - 1);
            position[number * 3 + 2] = height;

            center[number * 3 + 0] = position[number * 3 + 0] / 4;
            center[number * 3 + 1] = -centerY / 2.0f + (float)centerY * i / (image_row - 1);
            center[number * 3 + 2] = height;

            color[number * 4 + 0] = (float)colorImage->getPixel(i, j, RED) / 255.0f;
            color[number * 4 + 1] = (float)colorImage->getPixel(i, j, GREEN) / 255.0f;
            color[number * 4 + 2] = (float)colorImage->getPixel(i, j, BLUE) / 255.0f;
            color[number * 4 + 3] = 1.0;  // alpha
            ++number;
        }
    }
}


//did nothing here
// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

/////////////// bind programs/////// getting location ////// for position and color /////////////
GLuint program;
int restartIndex;
GLuint uiVBOData, uiVBOIndices;
GLuint vao;
void bindProgram() {
	pipelineProgram->Bind();
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiVBOIndices);
	glEnable(GL_PRIMITIVE_RESTART); 
	glPrimitiveRestartIndex(restartIndex);

	program = pipelineProgram->GetProgramHandle();
	glBindBuffer(GL_ARRAY_BUFFER, uiVBOData);

    /////////////position//////////
	GLuint loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc,3,GL_FLOAT,GL_FALSE,0,0);
    //////////////color////////////////////
	GLuint col = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(col);
	const void* offset2 = (const void*)(sizeof(GLfloat) * 3 * total_vertex);
	glVertexAttribPointer(col,4,GL_FLOAT,GL_FALSE,0,offset2);
}

//render some stuff here..................
void displayFunc()
{
  // render some stuff...
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    ///////////////////////////////////////////////////////

	bindProgram();

    ///////////////////// project //////////////////////
    matrix->SetMatrixMode(OpenGLMatrix::Projection);
    matrix->GetMatrix(p);
    //glUniform1i(glGetUniformLocation(program, "projectionMatrix"),0)
    glUniformMatrix4fv(glGetUniformLocation(program, "projectionMatrix"), 1, GL_FALSE, p);

    /////////////////// modelview ///////////////////
    matrix->SetMatrixMode(OpenGLMatrix::ModelView);
    matrix->LoadIdentity();
    matrix->LookAt(0.0, 0.0, 8.0,
        0.0, 0.0, 2.0,
        1.0, 0.0, 0.0);
    matrix->Translate(landTranslate[0], landTranslate[1], landTranslate[2]);  //translate
    matrix->Rotate(landRotate[0], 1, 0.0, 0.0);                               // rotate on x-axis
    matrix->Rotate(landRotate[1], 0.0, 1, 0.0);                               // rotate on y-axis
    matrix->Rotate(landRotate[2], 0.0, 0.0, 1);                               // rotate on z-axis
    matrix->Scale(landScale[0], landScale[1], landScale[2]);  //scale
    matrix->GetMatrix(m);
    glUniformMatrix4fv(glGetUniformLocation(program, "modelViewMatrix"), 1, GL_FALSE, m);
	
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// rendering //////////////////////////////////
    if (renderMode == POINT_MODE) {
        glDrawArrays(GL_POINTS, 0, total_vertex);  // points
    }
    else if (renderMode == LINE_MODE) {
        glDrawElements(GL_LINE_STRIP, (image_row - 1) * image_col * 2 + image_row - 2, GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe
        //glDrawElements(GL_LINE_STRIP, (image_row - 1)*image_col * 2 + image_row - 2, GL_UNSIGNED_INT, 0);
    }
    else if (renderMode == TRIANGLE_MODE) {
        glDrawElements(GL_TRIANGLE_STRIP, (image_row - 1) * image_col * 2 + image_row - 2, GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // solid triangle
    }
    else if (renderMode == SMOOTH_MODE) {
        glShadeModel(GL_SMOOTH);
        glDrawElements(GL_TRIANGLE_STRIP, ((image_row * 4 - 1) * image_col * 4 * 2 + image_row * 4 - 2) / 4, GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // solid triangle
    }
	glBindVertexArray(0); 
	glutSwapBuffers();
}

///////////////// animation ///////////////////////////
const int max_screenshots = 300;
int num_screenshots = 0;
bool take_screenshots = false;
void idleFunc()
{
  // do some stuff... 
  // for example, here, you can save the screenshots to disk (to make the animation)
  // make the screen update
    if (num_screenshots < 300) {
        if (num_screenshots < 75) {
            renderMode = POINT_MODE;
            //displayType = POINTS;
            //translate on x, y, and z axes
            if (num_screenshots < 12) {
                landTranslate[0] += 2.0f;
            }
            else if (num_screenshots < 25) {
                landTranslate[0] -= 2.0f;
            }
            else if (num_screenshots < 37) {
                landTranslate[1] += 2.0f;
            }
            else if (num_screenshots < 50) {
                landTranslate[1] -= 2.0f;
            }
            else if (num_screenshots < 62) {
                landTranslate[2] += 2.0f;
            }
            else {
                landTranslate[2] -= 2.0f;
            }
        }
        else if (num_screenshots < 150) {
            renderMode = LINE_MODE;
            landScale[0] = 1.2f;
            landScale[1] = 1.2f;
            landScale[2] = 1.2f;
            if (num_screenshots < 125) {
                landRotate[0] += (float)(360 / 50);
            }
            else {
                landRotate[1] += (float)(360 / 50);
            }
        }
        else if (num_screenshots < 225) {
            renderMode = TRIANGLE_MODE;
            if (num_screenshots < 175) {
                landRotate[1] += (float)(360 / 50);
            }
            else {
                landRotate[2] += (float)(360 / 50);
            }
        }
        //smooth
        else {
            renderMode = SMOOTH_MODE;
            //scale up and down to show the wireframe overlay on top of triangles
            if (num_screenshots < 263) {
                landScale[0] += 0.1f;
                landScale[1] += 0.1f;
                landScale[2] += 0.1f;
            }
            else {
                landScale[0] -= 0.1f;
                landScale[1] -= 0.1f;
                landScale[2] -= 0.1f;
            }
        }
        char animi_[5];
        sprintf(animi_, "%03d", num_screenshots);
        saveScreenshot(("./animation/" + std::string(animi_) + ".jpg").c_str());
        num_screenshots++;


    }

    glutPostRedisplay();
}

//did nothing here
void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  matrix->SetMatrixMode(OpenGLMatrix::Projection);
  matrix->LoadIdentity();
  matrix->Perspective(45.0, (float)w / (float)h, 0.1, 1000.0);
  matrix->SetMatrixMode(OpenGLMatrix::ModelView);

  //programPipeline//
}
//did nothing here
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
//did nothing here
void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}
//did nothing here
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
// keyboard input
void keyboardFunc(unsigned char key, int x, int y)
{
	
	  key = tolower(key); 
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
          saveScreenshot("screenshot.jpg");
        break;

        ////////////////////////////////// mode //////////////////////////
	    case '1' :  
		    renderMode = POINT_MODE;
		    cout << "point mode" << endl;
	    break;

	    case '2': 
		    renderMode = LINE_MODE;
		    cout << "wireframe mode" << endl;
	    break;

	    case '3':  
		    renderMode = TRIANGLE_MODE;
		    cout << "triangle mode" << endl;
	    break;
        case '4':
            renderMode = SMOOTH_MODE;
            cout << "smooth mode" << endl;
            break;

        ////////////////////////////////// trandformation ////////////////////////
        case 's':
            controlState = SCALE;
            cout << "scale" << endl;
            break;

        case 't':
            controlState = TRANSLATE;
            cout << "translate" << endl;
            break;
        case 'r':
            controlState = ROTATE;
            cout << "rotate" << endl;
            break;

      }
}

//////////// functional function /////////////////////////////
///////////////////////////////////////////////
// provide index to every vertex

GLuint* indices;
void initIndex() {

	int number = 0;
	restartIndex = image_row * image_col; 
	indices = (GLuint*)malloc(sizeof(GLuint)*((image_row -1)* image_col *2 + image_row -2));
	
	for (int i = 0; i < image_row - 1; ++i) {
		for (int j = 0; j < image_col; ++j) {
			for (int k = 0; k < 2; ++k) {
				int iRow = i + k;
				int index = iRow * image_col + j;
				indices[number] = index;
				++number;
			}
		}
		if (i != image_row - 2) { 
			indices[number] = restartIndex;
			++number;
		}
	}
}


void initPipelineProgram() {
	pipelineProgram = new BasicPipelineProgram();
	pipelineProgram->Init("../openGLHelper-starterCode");
}

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  // do additional initialization here...
  //////////////////////////////////////////////////////
  glEnable(GL_DEPTH_TEST);
  load_heightfleid();
  initIndex();
  matrix = new OpenGLMatrix();  

  //
  ////////////////////////////// bind buffer //////////////////////////////////
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &uiVBOData);
  glGenBuffers(1, &uiVBOIndices);
  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, uiVBOData);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * total_vertex + sizeof(GLfloat) * 4 * total_vertex, NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * 3 * total_vertex, position);
  //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * 3 * total_vertex, center);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * total_vertex, sizeof(GLfloat) * 4 * total_vertex, color);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiVBOIndices);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * ((image_row - 1) * image_col * 2 + image_row - 2), indices, GL_STATIC_DRAW);

  glBindVertexArray(0);
  ////////////////////////////////////////////////////////////////////////

  
  pipelineProgram = new BasicPipelineProgram();
  pipelineProgram->Init("../openGLHelper-starterCode");

}
//did nothine here
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
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  //light

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
      if (result != GLEW_OK) {
          cout << "error: " << glewGetErrorString(result) << endl;
          exit(EXIT_FAILURE);
      }
    #endif

      // do initialization
      initScene(argc, argv);

      // sink forever into the glut loop
      glutMainLoop();
}

