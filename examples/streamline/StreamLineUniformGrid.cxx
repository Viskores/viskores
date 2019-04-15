//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Initialize.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/StreamLineUniformGrid.h>

#include <vtkm/cont/testing/Testing.h>

#include <fstream>
#include <math.h>
#include <vector>

//Suppress warnings about glut being deprecated on OSX
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "../isosurface/quaternion.h"

// Output data set shared with opengl
static vtkm::worklet::StreamLineFilterUniformGrid<vtkm::Float32>* streamLineFilter;
static vtkm::cont::DataSet outDataSet;

// Input parameters
const vtkm::Id nSeeds = 25;
const vtkm::Id nSteps = 2000;
const vtkm::Float32 tStep = 0.5f;
const vtkm::Id direction = vtkm::worklet::streamline::BOTH;

// Point location of vertices from a CastAndCall but needs a static cast eventually
static vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> vertexArray;

// OpenGL display variables
Quaternion qrot;
int lastx, lasty;
int mouse_state = 1;

//
// Initialize the OpenGL state
//
void initializeGL()
{
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);

  float white[] = { 0.8f, 0.8f, 0.8f, 1.0f };
  float black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
  float lightPos[] = { 10.0f, 10.0f, 10.5f, 1.0f };

  glLightfv(GL_LIGHT0, GL_AMBIENT, white);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
  glLightfv(GL_LIGHT0, GL_SPECULAR, black);
  glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
}

//
// Render the output using simple OpenGL
//
void displayCall()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0f, 1.0f, 1.0f, 100.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0f, 0.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
  glLineWidth(1.0f);

  glPushMatrix();
  float rotationMatrix[16];
  qrot.getRotMat(rotationMatrix);
  glMultMatrixf(rotationMatrix);
  glTranslatef(-0.5f, -0.5f, -0.5f);

  // Get the cell set, coordinate system and coordinate data
  vtkm::cont::CellSetExplicit<> cellSet;
  outDataSet.GetCellSet(0).CopyTo(cellSet);
  auto coordArray = outDataSet.GetCoordinateSystem().GetData();

  vtkm::Id numberOfCells = cellSet.GetNumberOfCells();
  vtkm::cont::ArrayCopy(coordArray, vertexArray);

  // Each cell is a polyline
  glColor3f(1.0f, 0.0f, 0.0f);
  for (vtkm::Id polyline = 0; polyline < numberOfCells; polyline++)
  {
    vtkm::Vec<vtkm::Id, nSteps> polylineIndices;
    vtkm::IdComponent numIndices = cellSet.GetNumberOfPointsInCell(polyline);
    cellSet.GetIndices(polyline, polylineIndices);

    glBegin(GL_LINE_STRIP);
    for (vtkm::IdComponent i = 0; i < numIndices; i++)
    {
      vtkm::Vec<vtkm::Float32, 3> pt = vertexArray.GetPortalConstControl().Get(polylineIndices[i]);
      glVertex3f(pt[0], pt[1], pt[2]);
    }
    glEnd();
  }
  glPopMatrix();
  glutSwapBuffers();
}

// Allow rotations of the view
void mouseMove(int x, int y)
{
  vtkm::Float32 dx = static_cast<vtkm::Float32>(x - lastx);
  vtkm::Float32 dy = static_cast<vtkm::Float32>(y - lasty);

  if (mouse_state == 0)
  {
    Quaternion newRotX;
    newRotX.setEulerAngles(-0.2f * dx * vtkm::Pi_180f(), 0.0f, 0.0f);
    qrot.mul(newRotX);

    Quaternion newRotY;
    newRotY.setEulerAngles(0.0f, 0.0f, -0.2f * dy * vtkm::Pi_180f());
    qrot.mul(newRotY);
  }
  lastx = x;
  lasty = y;

  glutPostRedisplay();
}

// Respond to mouse button
void mouseCall(int button, int state, int x, int y)
{
  if (button == 0)
    mouse_state = state;
  if ((button == 0) && (state == 0))
  {
    lastx = x;
    lasty = y;
  }
}

namespace
{

template <typename T>
VTKM_EXEC_CONT vtkm::Vec<T, 3> Normalize(vtkm::Vec<T, 3> v)
{
  T magnitude = static_cast<T>(sqrt(vtkm::dot(v, v)));
  T zero = static_cast<T>(0.0);
  T one = static_cast<T>(1.0);
  if (magnitude == zero)
    return vtkm::make_Vec(zero, zero, zero);
  else
    return one / magnitude * v;
}
}

// Run streamlines on a uniform grid of vector data
int main(int argc, char* argv[])
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  auto config = vtkm::cont::Initialize(argc, argv, opts);

  std::cout << "StreamLineUniformGrid Example" << std::endl;
  std::cout << "Parameters are [options] fileName [numSeeds maxSteps timeStep direction]"
            << std::endl
            << std::endl;
  std::cout << "Direction is FORWARD=0 BACKWARD=1 BOTH=2" << std::endl << std::endl;
  std::cout << "File is expected to be binary with xdim ydim zdim as 32 bit integers " << std::endl;
  std::cout << "followed by vector data per dimension point as 32 bit float" << std::endl;
  std::cout << config.Usage << std::endl;

  // Read in the vector data for testing
  FILE* pFile = fopen(argv[1], "rb");
  if (pFile == nullptr)
    perror("Error opening file");

  size_t ret_code = 0;
  // Size of the dataset
  int dims[3];
  ret_code = fread(dims, sizeof(int), 3, pFile);
  if (ret_code != 3)
  {
    perror("Error reading size of data");
    fclose(pFile);
    return 0;
  }
  const vtkm::Id3 vdims(dims[0], dims[1], dims[2]);

  // Read vector data at each point of the uniform grid and store
  vtkm::Id nElements = vdims[0] * vdims[1] * vdims[2] * 3;
  float* data = new float[static_cast<std::size_t>(nElements)];
  ret_code = fread(data, sizeof(float), static_cast<std::size_t>(nElements), pFile);
  if (ret_code != static_cast<size_t>(nElements))
  {
    perror("Error reading vector data");
    fclose(pFile);
    return 0;
  }

  //We are done with the file now, so release the file descriptor
  fclose(pFile);

  std::vector<vtkm::Vec<vtkm::Float32, 3>> field;
  for (vtkm::Id i = 0; i < nElements; i++)
  {
    vtkm::Float32 x = data[i];
    vtkm::Float32 y = data[++i];
    vtkm::Float32 z = data[++i];
    vtkm::Vec<vtkm::Float32, 3> vecData(x, y, z);
    field.push_back(Normalize(vecData));
  }
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> fieldArray;
  fieldArray = vtkm::cont::make_ArrayHandle(field);

  // Construct the input dataset (uniform) to hold the input and set vector data
  vtkm::cont::DataSet inDataSet;
  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims);
  inDataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));
  inDataSet.AddField(
    vtkm::cont::Field("vecData", vtkm::cont::Field::Association::POINTS, fieldArray));

  vtkm::cont::CellSetStructured<3> inCellSet("cells");
  inCellSet.SetPointDimensions(vtkm::make_Vec(vdims[0], vdims[1], vdims[2]));
  inDataSet.AddCellSet(inCellSet);

  // Create and run the filter
  streamLineFilter = new vtkm::worklet::StreamLineFilterUniformGrid<vtkm::Float32>();
  outDataSet = streamLineFilter->Run(inDataSet, direction, nSeeds, nSteps, tStep);

  // Render the output dataset of polylines
  lastx = lasty = 0;
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(1000, 1000);

  glutCreateWindow("VTK-m Uniform 3D StreamLines");

  initializeGL();

  glutDisplayFunc(displayCall);

  glutMotionFunc(mouseMove);
  glutMouseFunc(mouseCall);
  glutMainLoop();

  delete streamLineFilter;
  outDataSet.Clear();
  vertexArray.ReleaseResources();
  return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif
