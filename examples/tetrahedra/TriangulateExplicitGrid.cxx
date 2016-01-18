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
#include <vtkm/cont/CellSetExplicit.h>
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

typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

namespace {

// Takes input uniform grid and outputs unstructured grid of triangles
vtkm::worklet::TetrahedralizeFilterExplicitGrid<DeviceAdapter> *tetrahedralizeFilter;
vtkm::cont::DataSet outDataSet;
vtkm::Id numberOfInPoints;

// Point location of vertices from a CastAndCall but needs a static cast eventually
vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3> > vertexArray;

} // anonymous namespace

//
// Construct an input data set with uniform grid of indicated dimensions, origin and spacing
//
vtkm::cont::DataSet MakeTriangulateExplicitDataSet()
{
  vtkm::cont::DataSetBuilderExplicitIterative builder;
  builder.Begin(2);

  builder.AddPoint(0, 0, 0); // 0
  builder.AddPoint(1, 0, 0); // 1
  builder.AddPoint(2, 0, 0); // 2
  builder.AddPoint(3, 0, 0); // 3
  builder.AddPoint(0, 1, 0); // 4
  builder.AddPoint(1, 1, 0); // 5
  builder.AddPoint(2, 1, 0); // 6
  builder.AddPoint(3, 1, 0); // 7
  builder.AddPoint(0, 2, 0); // 8
  builder.AddPoint(1, 2, 0); // 9
  builder.AddPoint(2, 2, 0); // 10
  builder.AddPoint(3, 2, 0); // 11
  builder.AddPoint(0, 3, 0); // 12
  builder.AddPoint(3, 3, 0); // 13
  builder.AddPoint(1, 4, 0); // 14
  builder.AddPoint(2, 4, 0); // 15

  builder.AddCell(vtkm::CELL_SHAPE_TRIANGLE);
  builder.AddCellPoint(0);
  builder.AddCellPoint(1);
  builder.AddCellPoint(5);

  builder.AddCell(vtkm::CELL_SHAPE_QUAD);
  builder.AddCellPoint(1);
  builder.AddCellPoint(2);
  builder.AddCellPoint(6);
  builder.AddCellPoint(5);

  builder.AddCell(vtkm::CELL_SHAPE_QUAD);
  builder.AddCellPoint(5);
  builder.AddCellPoint(6);
  builder.AddCellPoint(10);
  builder.AddCellPoint(9);

  builder.AddCell(vtkm::CELL_SHAPE_QUAD);
  builder.AddCellPoint(4);
  builder.AddCellPoint(5);
  builder.AddCellPoint(9);
  builder.AddCellPoint(8);

  builder.AddCell(vtkm::CELL_SHAPE_TRIANGLE);
  builder.AddCellPoint(2);
  builder.AddCellPoint(3);
  builder.AddCellPoint(7);

  builder.AddCell(vtkm::CELL_SHAPE_QUAD);
  builder.AddCellPoint(6);
  builder.AddCellPoint(7);
  builder.AddCellPoint(11);
  builder.AddCellPoint(10);

  builder.AddCell(vtkm::CELL_SHAPE_POLYGON);
  builder.AddCellPoint(9);
  builder.AddCellPoint(10);
  builder.AddCellPoint(13);
  builder.AddCellPoint(15);
  builder.AddCellPoint(14);
  builder.AddCellPoint(12);

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
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5f, 3.5f, -0.5f, 4.5f, -1.0f, 1.0f);
}


//
// Render the output using simple OpenGL
//
void displayCall()
{
  glClear(GL_COLOR_BUFFER_BIT);
  glLineWidth(3.0f);

  // Get cell set and the number of cells and vertices
  vtkm::cont::CellSetSingleType<> cellSet =
              outDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetSingleType<> >();
  vtkm::Id numberOfCells = cellSet.GetNumberOfCells();

  // Get the coordinate system and coordinate data
  const vtkm::cont::DynamicArrayHandleCoordinateSystem coordArray =
                                      outDataSet.GetCoordinateSystem(0).GetData();

  // Need the actual vertex points from a static cast of the dynamic array but can't get it right
  // So use cast and call on a functor that stores that dynamic array into static array we created
  vertexArray.Allocate(numberOfInPoints);
  coordArray.CastAndCall(GetVertexArray());

  // Draw the two triangles belonging to each quad
  vtkm::Float32 color[4][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 0.0f}
  };

  for (vtkm::Id triangle = 0; triangle < numberOfCells; triangle++) {
    vtkm::Id indx = triangle % 4;
    glColor3f(color[indx][0], color[indx][1], color[indx][2]);

    // Get the indices of the vertices that make up this triangle
    vtkm::Vec<vtkm::Id, 3> triIndices;
    cellSet.GetIndices(triangle, triIndices);

    // Get the vertex points for this triangle
    vtkm::Vec<vtkm::Float64,3> pt0 = vertexArray.GetPortalConstControl().Get(triIndices[0]);
    vtkm::Vec<vtkm::Float64,3> pt1 = vertexArray.GetPortalConstControl().Get(triIndices[1]);
    vtkm::Vec<vtkm::Float64,3> pt2 = vertexArray.GetPortalConstControl().Get(triIndices[2]);

    // Draw the triangle filled with alternating colors
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glBegin(GL_TRIANGLES);
      glVertex3d(pt0[0], pt0[1], pt0[2]);
      glVertex3d(pt1[0], pt1[1], pt1[2]);
      glVertex3d(pt2[0], pt2[1], pt2[2]);
    glEnd();
  }
  glFlush();
}

// Triangulate and render explicit grid example
int main(int argc, char* argv[])
{
  std::cout << "TrianguleExplicitGrid Example" << std::endl;

  // Create the input uniform cell set
  vtkm::cont::DataSet inDataSet = MakeTriangulateExplicitDataSet();
  vtkm::cont::CellSetExplicit<> &inCellSet =
      inDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetExplicit<> >();

  numberOfInPoints = inCellSet.GetNumberOfPoints();

  // Create the output dataset explicit cell set with same coordinate system
  vtkm::cont::CellSetSingleType<> cellSet(vtkm::CellShapeTagTriangle(), "cells");;
  outDataSet.AddCellSet(cellSet);
  outDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(0));

  // Convert 2D explicit cells to triangles
  tetrahedralizeFilter = new vtkm::worklet::TetrahedralizeFilterExplicitGrid<DeviceAdapter>
                                              (inDataSet, outDataSet);
  tetrahedralizeFilter->Run();

  // Render the output dataset of tets
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
  glutInitWindowSize(1000, 1000);
  glutInitWindowPosition(100, 100);

  glutCreateWindow("VTK-m Explicit Triangulate");

  initializeGL();

  glutDisplayFunc(displayCall);
  glutMainLoop();

  return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
# pragma GCC diagnostic pop
#endif
