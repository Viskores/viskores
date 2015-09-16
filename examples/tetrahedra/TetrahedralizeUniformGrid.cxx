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
vtkm::Id cellsToDisplay = 64;

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

  vtkm::Id cameraDistance = dims[0] + dims[1] + dims[2];

  gluLookAt(static_cast<vtkm::Float32>(dims[0] / 2),
            static_cast<vtkm::Float32>(dims[1] / 2),
            static_cast<vtkm::Float32>(cameraDistance),

            static_cast<vtkm::Float32>(dims[0] / 2),
            static_cast<vtkm::Float32>(dims[1] / 2),
            0.0f, 0.0f, 1.0f, 0.0f);
  glLineWidth(3.0f);

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

  // Need the actual coordinate that matches the indices but can't figure out how to get
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3> > dataArray;
  dataArray.Allocate(numberOfPoints);
  vtkm::Id index = 0;
  for (vtkm::Id k = 0; k < dims[2] + 1; k++) {
    for (vtkm::Id j = 0; j < dims[1] + 1; j++) {
      for (vtkm::Id i = 0; i < dims[0] + 1; i++) {
        vtkm::Vec<vtkm::Float64, 3> location = vtkm::make_Vec(static_cast<vtkm::Float64>(i),
                                                              static_cast<vtkm::Float64>(j),
                                                              static_cast<vtkm::Float64>(k));
std::cout << "Location index " << index << "  Location: " << location << std::endl;
        dataArray.GetPortalControl().Set(index++, location);
      }
    }
  }
  // DataSet has a CoordinateSystem
  // CoordinateSystem is a Field which has data
  // Data in the Field is a DynamicArrayHandleCoordinateSystem
  // which is a DynamicArrayHandleBase
  // which is a PolymorphicArrayHandleContainerBase
  // which is where the PrintSummary is that prints my points
  // because at that point it is a simple ArrayHandle with a GetPortalConstControl()
  //
  vtkm::cont::CoordinateSystem coordinates = outDataSet.GetCoordinateSystem(0);
  coordinates.PrintSummary(std::cout);

  // This should be the actual values of the points
  vtkm::cont::DynamicArrayHandleCoordinateSystem coordArray = coordinates.GetData();
  vtkm::Id numberOfValues = coordArray.GetNumberOfValues();
  vtkm::Id numberOfComponents = coordArray.GetNumberOfComponents();
  vtkm::cont::ArrayHandle<vtkm::Float64> bounds = coordinates.GetBounds(DeviceAdapter());
  printf("Coordinate system num values %ld num comp %ld\n",numberOfValues, numberOfComponents);
  printf("Bounds (%lf, %lf) (%lf, %lf) (%lf, %lf)\n",
                                 bounds.GetPortalControl().Get(0), bounds.GetPortalControl().Get(1),
                                 bounds.GetPortalControl().Get(2), bounds.GetPortalControl().Get(3),
                                 bounds.GetPortalControl().Get(4), bounds.GetPortalControl().Get(5));

  // In the loop I can get the number of points in each cell and the cell shape
  // I can also get the Indices into the points that make up the shape
  // I can get the bounds which are correct
  // If I PrintSummary on the CoordinateSystem I get the points [0,0,0][1,0,0]...[4,4,4]
  // so I know they are there
  // What is the cast I need to turn this into a simple ArrayHandle?  

  // Draw the tetrahedra (this must change from the isosurface)
  vtkm::Id numberOfHexahedra = numberOfCells / 5;
  vtkm::Id tetra = 0;
  vtkm::Float32 color[5][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 0.0f}};
  for (vtkm::Id hex = 0; hex < cellsToDisplay; hex++)
  {
    for (vtkm::Id j = 0; j < 5; j++) {
      vtkm::Id indx = tetra % 5;
std::cout << "COLORS " << color[indx][0] << ", " << color[indx][1] << ", " << color[indx][2] << std::endl;
      glColor3f(color[indx][0], color[indx][1], color[indx][2]);

      vtkm::Id pointsInCell = cellSet.GetNumberOfPointsInCell(tetra);
      vtkm::Id cellShape = cellSet.GetCellShape(tetra);
      printf("Hex %ld Tet %ld number of points %ld shape %ld\n", hex, tetra, pointsInCell, cellShape);
      vtkm::Vec<vtkm::Id, 4> tetIndices;
      cellSet.GetIndices(tetra, tetIndices);
      printf("Tet %ld indices %ld %ld %ld %ld\n", tetra, tetIndices[0], tetIndices[1], tetIndices[2], tetIndices[3]);

      std::cout << "Index 0 " << dataArray.GetPortalConstControl().Get(tetIndices[0]) << std::endl;
      std::cout << "Index 1 " << dataArray.GetPortalConstControl().Get(tetIndices[1]) << std::endl;
      std::cout << "Index 2 " << dataArray.GetPortalConstControl().Get(tetIndices[2]) << std::endl;
      std::cout << "Index 3 " << dataArray.GetPortalConstControl().Get(tetIndices[3]) << std::endl;

      vtkm::Vec<vtkm::Float64,3> pt0 = dataArray.GetPortalConstControl().Get(tetIndices[0]);
      vtkm::Vec<vtkm::Float64,3> pt1 = dataArray.GetPortalConstControl().Get(tetIndices[1]);
      vtkm::Vec<vtkm::Float64,3> pt2 = dataArray.GetPortalConstControl().Get(tetIndices[2]);
      vtkm::Vec<vtkm::Float64,3> pt3 = dataArray.GetPortalConstControl().Get(tetIndices[3]);

/*
      std::cout << "Point 0 " << pt0 << std::endl;
      std::cout << "Point 1 " << pt1 << std::endl;
      std::cout << "Point 2 " << pt2 << std::endl;
      std::cout << "Point 3 " << pt3 << std::endl;
*/

      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
      glBegin(GL_TRIANGLE_STRIP);
      glVertex3d(pt0[0], pt0[1], pt0[2]);
      glVertex3d(pt1[0], pt1[1], pt1[2]);
      glVertex3d(pt2[0], pt2[1], pt2[2]);
      glVertex3d(pt3[0], pt3[1], pt3[2]);
      glVertex3d(pt0[0], pt0[1], pt0[2]);
      glVertex3d(pt1[0], pt1[1], pt1[2]);
      glEnd();

      glColor3f(1.0f, 1.0f, 1.0f);
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
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
  std::cout << "Parameters are [xdim ydim zdim [cellsToDisplay]]" << std::endl;
  
  // Set the problem size and number of cells to display from command line
  if (argc >= 4) {
    dims[0] = atoi(argv[1]);
    dims[1] = atoi(argv[2]);
    dims[2] = atoi(argv[3]);
    cellsToDisplay = dims[0] * dims[1] * dims[2];
  }
  if (argc == 5) {
    cellsToDisplay = atoi(argv[4]);
  }
  std::cout << "Dimension of problem: " << dims[0] << ":" << dims[1] << ":" << dims[2] << std::endl;
  std::cout << "Number of hexahedra to display: " << cellsToDisplay << std::endl;

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
