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

#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif

#include <vtkm/worklet/TetrahedralizeExplicitGrid.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/Math.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>

#include <vtkm/cont/testing/Testing.h>

//Suppress warnings about glut being deprecated on OSX
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#if defined (__APPLE__)
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
#endif

#include "../isosurface/quaternion.h"

typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

namespace {

// Takes input uniform grid and outputs unstructured grid of tets
vtkm::cont::DataSet outDataSet;
vtkm::Id numberOfInPoints;

// Point location of vertices from a CastAndCall but needs a static cast eventually
vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3> > vertexArray;

// OpenGL display variables
Quaternion qrot;
int lastx, lasty;
int mouse_state = 1;

} // anonymous namespace

//
// Test 3D explicit dataset
//
vtkm::cont::DataSet MakeTetrahedralizeExplicitDataSet()
{
  vtkm::cont::DataSetBuilderExplicitIterative builder;
  builder.Begin(3);

  builder.AddPoint( 0, 0, 0);
  builder.AddPoint( 1, 0, 0);
  builder.AddPoint( 2, 0, 0);
  builder.AddPoint( 3, 0, 0);
  builder.AddPoint( 0, 1, 0);
  builder.AddPoint( 1, 1, 0);
  builder.AddPoint( 2, 1, 0);
  builder.AddPoint( 2.5, 1.0, 0.0);
  builder.AddPoint( 0, 2, 0);
  builder.AddPoint( 1, 2, 0);
  builder.AddPoint( 0.5, 0.5, 1.0);
  builder.AddPoint( 1, 0, 1);
  builder.AddPoint( 2, 0, 1);
  builder.AddPoint( 3, 0, 1);
  builder.AddPoint( 1, 1, 1);
  builder.AddPoint( 2, 1, 1);
  builder.AddPoint( 2.5, 1.0, 1.0);
  builder.AddPoint( 0.5, 1.5, 1.0);

  builder.AddCell(vtkm::CELL_SHAPE_TETRA);
  builder.AddCellPoint(0);
  builder.AddCellPoint(1);
  builder.AddCellPoint(5);
  builder.AddCellPoint(10);

  builder.AddCell(vtkm::CELL_SHAPE_HEXAHEDRON);
  builder.AddCellPoint(1);
  builder.AddCellPoint(2);
  builder.AddCellPoint(6);
  builder.AddCellPoint(5);
  builder.AddCellPoint(11);
  builder.AddCellPoint(12);
  builder.AddCellPoint(15);
  builder.AddCellPoint(14);

  builder.AddCell(vtkm::CELL_SHAPE_WEDGE);
  builder.AddCellPoint(2);
  builder.AddCellPoint(3);
  builder.AddCellPoint(7);
  builder.AddCellPoint(12);
  builder.AddCellPoint(13);
  builder.AddCellPoint(16);

  builder.AddCell(vtkm::CELL_SHAPE_PYRAMID);
  builder.AddCellPoint(4);
  builder.AddCellPoint(5);
  builder.AddCellPoint(9);
  builder.AddCellPoint(8);
  builder.AddCellPoint(17);

  return builder.Create();
}

//
// Functor to retrieve vertex locations from the CoordinateSystem
// Actually need a static cast to ArrayHandle from DynamicArrayHandleCoordinateSystem
// but haven't been able to figure out what that is
//
struct GetVertexArray
{
  template <typename ArrayHandleType>
  VTKM_CONT_EXPORT
  void operator()(ArrayHandleType array) const
  {
    this->GetVertexPortal(array.GetPortalConstControl());
  }

private:
  template <typename PortalType>
  VTKM_CONT_EXPORT
  void GetVertexPortal(const PortalType &portal) const
  {
    for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      vertexArray.GetPortalControl().Set(index, portal.Get(index));
    }
  }
};

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
  gluPerspective( 45.0f, 1.0f, 1.0f, 40.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(1.5f, 2.0f, 6.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  glPushMatrix();
  float rotationMatrix[16];
  qrot.getRotMat(rotationMatrix);
  glMultMatrixf(rotationMatrix);
  glTranslatef(-0.5f, -0.5f, -0.5f);

  // Get cell set and the number of cells and vertices
  vtkm::cont::CellSetSingleType<> cellSet;
  outDataSet.GetCellSet(0).CopyTo(cellSet);
  vtkm::Id numberOfCells = cellSet.GetNumberOfCells();

  // Get the coordinate system and coordinate data
  const vtkm::cont::DynamicArrayHandleCoordinateSystem coordArray =
                                      outDataSet.GetCoordinateSystem(0).GetData();

  // Need the actual vertex points from a static cast of the dynamic array but can't get it right
  // So use cast and call on a functor that stores that dynamic array into static array we created
  vertexArray.Allocate(numberOfInPoints);
  coordArray.CastAndCall(GetVertexArray());

  // Draw the five tetrahedra belonging to each hexadron
  vtkm::Float32 color[5][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 0.0f}
  };

  for (vtkm::Id tetra = 0; tetra < numberOfCells; tetra++) {
    vtkm::Id indx = tetra % 5;
    glColor3f(color[indx][0], color[indx][1], color[indx][2]);

    // Get the indices of the vertices that make up this tetrahedron
    vtkm::Vec<vtkm::Id, 4> tetIndices;
    cellSet.GetIndices(tetra, tetIndices);

    // Get the vertex points for this tetrahedron
    vtkm::Vec<vtkm::Float64,3> pt0 = vertexArray.GetPortalConstControl().Get(tetIndices[0]);
    vtkm::Vec<vtkm::Float64,3> pt1 = vertexArray.GetPortalConstControl().Get(tetIndices[1]);
    vtkm::Vec<vtkm::Float64,3> pt2 = vertexArray.GetPortalConstControl().Get(tetIndices[2]);
    vtkm::Vec<vtkm::Float64,3> pt3 = vertexArray.GetPortalConstControl().Get(tetIndices[3]);

    // Draw the tetrahedron filled with alternating colors
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
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
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    glBegin(GL_TRIANGLE_STRIP);
    glVertex3d(pt0[0], pt0[1], pt0[2]);
    glVertex3d(pt1[0], pt1[1], pt1[2]);
    glVertex3d(pt2[0], pt2[1], pt2[2]);
    glVertex3d(pt3[0], pt3[1], pt3[2]);
    glVertex3d(pt0[0], pt0[1], pt0[2]);
    glVertex3d(pt1[0], pt1[1], pt1[2]);
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
    vtkm::Float32 pi = static_cast<float>(vtkm::Pi());
    Quaternion newRotX;
    newRotX.setEulerAngles(-0.2f * dx * pi / 180.0f, 0.0f, 0.0f);
    qrot.mul(newRotX);

    Quaternion newRotY;
    newRotY.setEulerAngles(0.0f, 0.0f, -0.2f * dy * pi / 180.0f);
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
  std::cout << "TetrahedralizeExplicitGrid Example" << std::endl;

  // Create the input explicit cell set
  vtkm::cont::DataSet inDataSet = MakeTetrahedralizeExplicitDataSet();
  vtkm::cont::CellSetExplicit<> inCellSet;
  inDataSet.GetCellSet(0).CopyTo(inCellSet);

  numberOfInPoints = inCellSet.GetNumberOfPoints();

  // Create the output dataset explicit cell set with same coordinate system
  vtkm::cont::CellSetSingleType<> cellSet(vtkm::CellShapeTagTetra(), "cells");
  outDataSet.AddCellSet(cellSet);
  outDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(0));

  // Convert cells to tetrahedra
  vtkm::worklet::TetrahedralizeFilterExplicitGrid<DeviceAdapter>
                 tetrahedralizeFilter(inDataSet, outDataSet);
  tetrahedralizeFilter.Run();

  // Render the output dataset of tets
  lastx = lasty = 0;
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(1000, 1000);

  glutCreateWindow("VTK-m Explicit Tetrahedralize");

  initializeGL();

  glutDisplayFunc(displayCall);

  glutMotionFunc(mouseMove);
  glutMouseFunc(mouseCall);
  glutMainLoop();

  return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
# pragma GCC diagnostic pop
#endif
