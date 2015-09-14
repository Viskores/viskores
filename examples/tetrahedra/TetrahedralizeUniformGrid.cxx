//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/TetrahedralizeUniformGrid.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/testing/Testing.h>

#if defined (__APPLE__)
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
#endif

#include "quaternion.h"

typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

vtkm::Id3 dims(4,4,4);

vtkm::worklet::TetrahedralizeFilterUniformGrid<DeviceAdapter> *tetrahedralizeFilter;

vtkm::cont::DataSet outDataSet;
Quaternion qrot;
int lastx, lasty;
int mouse_state = 1;

namespace {

// Construct an input data set
vtkm::cont::DataSet MakeTetrahedralizeTestDataSet(vtkm::Id3 dims)
{
  vtkm::cont::DataSet dataSet;

  const vtkm::Id3 vdims(dims[0] + 1, dims[1] + 1, dims[2] + 1);

  float mins[3] = {-1.0f, -1.0f, -1.0f};
  float maxs[3] = {1.0f, 1.0f, 1.0f};

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims);
  dataSet.AddCoordinateSystem(
          vtkm::cont::CoordinateSystem("coordinates", 1, coordinates));

  static const vtkm::IdComponent ndim = 3;
  vtkm::cont::CellSetStructured<ndim> cellSet("cells");
  cellSet.SetPointDimensions(vdims);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

}


// Initialize the OpenGL state
void initializeGL()
{
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);

  float white[] = { 0.8, 0.8, 0.8, 1.0 };
  float black[] = { 0.0, 0.0, 0.0, 1.0 };
  float lightPos[] = { 10.0, 10.0, 10.5, 1.0 };

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


// Render the output using simple OpenGL
void displayCall()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective( 45.0f, 1.0f, 1.0f, 20.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  glPushMatrix();
  float rotationMatrix[16];
  qrot.getRotMat(rotationMatrix);
  glMultMatrixf(rotationMatrix);
  glTranslatef(-0.5f, -0.5f, -0.5f);
 
  glColor3f(0.1f, 0.1f, 0.6f);

  // Number of tetrahedra
  vtkm::cont::CellSetExplicit<> cellSet = outDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetExplicit<> >();
  vtkm::Id numberOfCells = cellSet.GetNumberOfCells();
  vtkm::Id numberOfPoints = cellSet.GetNumberOfPoints();
printf("Number of cells in GL set %ld\n", numberOfCells);
printf("Number of points in GL set %ld\n", numberOfPoints);

  // Draw the tetrahedra (this must change from the isosurface)
  glBegin(GL_TRIANGLES);
  for (vtkm::Id i = 0; i < numberOfCells; i++)
  {
    // Get a cell
    // Get the four vertices of the cell
    // Decompose into 4 triangles
    vtkm::Id pointsInCell = cellSet.GetNumberOfPointsInCell(i);
    vtkm::Id cellShape = cellSet.GetCellShape(i);
printf("Cell %ld number of points %ld shape %ld\n", i, pointsInCell, cellShape);
    vtkm::Vec<vtkm::Id, 4> tetIndices;
    cellSet.GetIndices(i, tetIndices);
printf("Tet %ld indices %ld %ld %ld %ld\n", i, tetIndices[0], tetIndices[1], tetIndices[2], tetIndices[3]);
/*
    vtkm::Vec<vtkm::Float32, 3> curNormal = normalsArray.GetPortalConstControl().Get(i);
    vtkm::Vec<vtkm::Float32, 3> curVertex = verticesArray.GetPortalConstControl().Get(i);
    glNormal3f(curNormal[0], curNormal[1], curNormal[2]);
    glVertex3f(curVertex[0], curVertex[1], curVertex[2]);
*/
  }
  glEnd();

  glPopMatrix();
  glutSwapBuffers();
}


// Allow rotations of the view
void mouseMove(int x, int y)
{
  int dx = x - lastx;
  int dy = y - lasty;

  if (mouse_state == 0)
  {
    Quaternion newRotX;
    newRotX.setEulerAngles(-0.2*dx*M_PI/180.0, 0.0, 0.0);
    qrot.mul(newRotX);

    Quaternion newRotY;
    newRotY.setEulerAngles(0.0, 0.0, -0.2*dy*M_PI/180.0);
    qrot.mul(newRotY);
  }
  lastx = x;
  lasty = y;

  glutPostRedisplay();
}


// Respond to mouse button
void mouseCall(int button, int state, int x, int y)
{
  if (button == 0) mouse_state = state;
  if ((button == 0) && (state == 0)) { lastx = x;  lasty = y; }
}


// Tetrahedralize and render uniform grid example
int main(int argc, char* argv[])
{
  std::cout << "TetrahedralizeUniformGrid Example" << std::endl;

  // Create the input uniform cell set
  vtkm::cont::DataSet inDataSet = MakeTetrahedralizeTestDataSet(dims);

  // Set number of cells and vertices in input dataset
  vtkm::Id numberOfCells = dims[0] * dims[1] * dims[2];
  vtkm::Id numberOfVertices = (dims[0] + 1) * (dims[1] + 1) * (dims[2] + 1);

  // Create the output dataset explicit cell set with same coordinate system
  vtkm::cont::CellSetExplicit<> cellSet(numberOfVertices, "cells", 3);
  outDataSet.AddCellSet(cellSet);
  outDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(0));

  // Convert uniform hexahedra to tetrahedra
  tetrahedralizeFilter = new vtkm::worklet::TetrahedralizeFilterUniformGrid<DeviceAdapter>(dims, inDataSet, outDataSet);
  tetrahedralizeFilter->Run();

  // Render the output dataset of tets
  lastx = lasty = 0;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(1000, 1000);

  glutCreateWindow("VTK-m Isosurface");

  initializeGL();

  glutDisplayFunc(displayCall);

  glutMotionFunc(mouseMove);
  glutMouseFunc(mouseCall);
  glutMainLoop();

  return 0;
}
