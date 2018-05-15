//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif

#include <vtkm/Math.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/filter/Tetrahedralize.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/cont/testing/Testing.h>

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

using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;

// Default size of the example
static vtkm::Id3 dims(4, 4, 4);
static vtkm::Id cellsToDisplay = 64;

// Takes input uniform grid and outputs unstructured grid of tets
static vtkm::cont::DataSet tetDataSet;

// OpenGL display variables
static Quaternion qrot;
static int lastx, lasty;
static int mouse_state = 1;

//
// Construct an input data set with uniform grid of indicated dimensions, origin and spacing
//
vtkm::cont::DataSet MakeTetrahedralizeTestDataSet(vtkm::Id3 dim)
{
  vtkm::cont::DataSet dataSet;

  // Place uniform grid on a set physical space so OpenGL drawing is easier
  const vtkm::Id3 vdims(dim[0] + 1, dim[1] + 1, dim[2] + 1);
  const vtkm::Vec<vtkm::Float32, 3> origin = vtkm::make_Vec(0.0f, 0.0f, 0.0f);
  const vtkm::Vec<vtkm::Float32, 3> spacing =
    vtkm::make_Vec(1.0f / static_cast<vtkm::Float32>(dim[0]),
                   1.0f / static_cast<vtkm::Float32>(dim[1]),
                   1.0f / static_cast<vtkm::Float32>(dim[2]));

  // Generate coordinate system
  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims, origin, spacing);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  // Generate cell set
  vtkm::cont::CellSetStructured<3> cellSet("cells");
  cellSet.SetPointDimensions(vdims);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

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
  gluPerspective(45.0f, 1.0f, 1.0f, 20.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
  glLineWidth(3.0f);

  glPushMatrix();
  float rotationMatrix[16];
  qrot.getRotMat(rotationMatrix);
  glMultMatrixf(rotationMatrix);
  glTranslatef(-0.5f, -0.5f, -0.5f);

  // Get the cell set, coordinate system and coordinate data
  vtkm::cont::CellSetSingleType<> cellSet;
  tetDataSet.GetCellSet(0).CopyTo(cellSet);

  auto vertexArray = tetDataSet.GetCoordinateSystem().GetData();

  // Draw the five tetrahedra belonging to each hexadron
  vtkm::Id tetra = 0;
  vtkm::Float32 color[5][3] = { { 1.0f, 0.0f, 0.0f },
                                { 0.0f, 1.0f, 0.0f },
                                { 0.0f, 0.0f, 1.0f },
                                { 1.0f, 0.0f, 1.0f },
                                { 1.0f, 1.0f, 0.0f } };

  for (vtkm::Id hex = 0; hex < cellsToDisplay; hex++)
  {
    for (vtkm::Id j = 0; j < 5; j++)
    {
      vtkm::Id indx = tetra % 5;
      glColor3f(color[indx][0], color[indx][1], color[indx][2]);

      // Get the indices of the vertices that make up this tetrahedron
      vtkm::Vec<vtkm::Id, 4> tetIndices;
      cellSet.GetIndices(tetra, tetIndices);

      // Get the vertex points for this tetrahedron
      auto pt0 = vertexArray.GetPortalConstControl().Get(tetIndices[0]);
      auto pt1 = vertexArray.GetPortalConstControl().Get(tetIndices[1]);
      auto pt2 = vertexArray.GetPortalConstControl().Get(tetIndices[2]);
      auto pt3 = vertexArray.GetPortalConstControl().Get(tetIndices[3]);

      // Draw the tetrahedron filled with alternating colors
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glBegin(GL_TRIANGLE_STRIP);
      glVertex3d(pt0[0], pt0[1], pt0[2]);
      glVertex3d(pt1[0], pt1[1], pt1[2]);
      glVertex3d(pt2[0], pt2[1], pt2[2]);
      glVertex3d(pt3[0], pt3[1], pt3[2]);
      glVertex3d(pt0[0], pt0[1], pt0[2]);
      glVertex3d(pt1[0], pt1[1], pt1[2]);
      glEnd();

      // Draw the tetrahedron wireframe
      glColor3f(1.0f, 1.0f, 1.0f);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glBegin(GL_TRIANGLE_STRIP);
      glVertex3d(pt0[0], pt0[1], pt0[2]);
      glVertex3d(pt1[0], pt1[1], pt1[2]);
      glVertex3d(pt2[0], pt2[1], pt2[2]);
      glVertex3d(pt3[0], pt3[1], pt3[2]);
      glVertex3d(pt0[0], pt0[1], pt0[2]);
      glVertex3d(pt1[0], pt1[1], pt1[2]);
      glEnd();

      tetra++;
    }
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

// Tetrahedralize and render uniform grid example
int main(int argc, char* argv[])
{
  std::cout << "TetrahedralizeUniformGrid Example" << std::endl;
  std::cout << "Parameters are [xdim ydim zdim [# of cellsToDisplay]]" << std::endl << std::endl;

  // Set the problem size and number of cells to display from command line
  if (argc >= 4)
  {
    dims[0] = atoi(argv[1]);
    dims[1] = atoi(argv[2]);
    dims[2] = atoi(argv[3]);
    cellsToDisplay = dims[0] * dims[1] * dims[2];
  }
  if (argc == 5)
  {
    cellsToDisplay = atoi(argv[4]);
  }

  // Create the input uniform cell set
  vtkm::cont::DataSet inDataSet = MakeTetrahedralizeTestDataSet(dims);

  vtkm::filter::Tetrahedralize tetrahedralize;
  tetDataSet = tetrahedralize.Execute(inDataSet);

  // Render the output dataset of tets
  lastx = lasty = 0;
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(1000, 1000);

  glutCreateWindow("VTK-m Uniform Tetrahedralize");

  initializeGL();

  glutDisplayFunc(displayCall);

  glutMotionFunc(mouseMove);
  glutMouseFunc(mouseCall);
  glutMainLoop();

  tetDataSet.Clear();
  return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif
