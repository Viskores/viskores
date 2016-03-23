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

#include <vtkm/worklet/TetrahedralizeUniformGrid.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/Math.h>
#include <vtkm/cont/DataSet.h>

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

// Default size of the example
vtkm::Id2 dims(4,4);
vtkm::Id cellsToDisplay = 16;
vtkm::Id numberOfInPoints;

// Takes input uniform grid and outputs unstructured grid of triangles
vtkm::worklet::TetrahedralizeFilterUniformGrid<DeviceAdapter> *tetrahedralizeFilter;
vtkm::cont::DataSet tetDataSet;

// Point location of vertices from a CastAndCall but needs a static cast eventually
vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3> > vertexArray;

//
// Construct an input data set with uniform grid of indicated dimensions, origin and spacing
//
vtkm::cont::DataSet MakeTriangulateTestDataSet(vtkm::Id2 dim)
{
  vtkm::cont::DataSet dataSet;

  // Place uniform grid on a set physical space so OpenGL drawing is easier
  const vtkm::Id3 vdims(dim[0] + 1, dim[1] + 1, 1);
  const vtkm::Vec<vtkm::Float32, 3> origin = vtkm::make_Vec(0.0f, 0.0f, 0.0f);
  const vtkm::Vec<vtkm::Float32, 3> spacing = vtkm::make_Vec(
                                              1.0f/static_cast<vtkm::Float32>(dim[0]),
                                              1.0f/static_cast<vtkm::Float32>(dim[1]),
                                              1.0f/static_cast<vtkm::Float32>(dim[2]));

  // Generate coordinate system
  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims, origin, spacing);
  dataSet.AddCoordinateSystem(
          vtkm::cont::CoordinateSystem("coordinates", coordinates));

  // Generate cell set
  vtkm::cont::CellSetStructured<2> cellSet("cells");
  cellSet.SetPointDimensions(vtkm::make_Vec(dim[0] + 1, dim[1] + 1));
  dataSet.AddCellSet(cellSet);

  return dataSet;
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
  glOrtho(-0.5f, 1.5f, -0.5f, 1.5f, -1.0f, 1.0f);
}


//
// Render the output using simple OpenGL
//
void displayCall()
{
  glClear(GL_COLOR_BUFFER_BIT);
  glLineWidth(3.0f);

  // Get the cellset, coordinate system and coordinate data
  vtkm::cont::CellSetSingleType<> cellSet;
  tetDataSet.GetCellSet(0).CopyTo(cellSet);
  const vtkm::cont::DynamicArrayHandleCoordinateSystem &coordArray =
                                      tetDataSet.GetCoordinateSystem(0).GetData();

  // Need the actual vertex points from a static cast of the dynamic array but can't get it right
  // So use cast and call on a functor that stores that dynamic array into static array we created
  vertexArray.Allocate(numberOfInPoints);
  coordArray.CastAndCall(GetVertexArray());

  // Draw the two triangles belonging to each quad
  vtkm::Id triangle = 0;
  vtkm::Float32 color[4][3] =
  {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 0.0f}
  };

  for (vtkm::Id quad = 0; quad < cellsToDisplay; quad++)
  {
    for (vtkm::Id j = 0; j < 2; j++)
    {
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

      triangle++;
    }
  }
  glFlush();
}

// Tetrahedralize and render uniform grid example
int main(int argc, char* argv[])
{
  std::cout << "TrianguleUniformGrid Example" << std::endl;
  std::cout << "Parameters are [xdim ydim [# of cellsToDisplay]]" << std::endl << std::endl;

  // Set the problem size and number of cells to display from command line
  if (argc >= 3)
  {
    dims[0] = atoi(argv[1]);
    dims[1] = atoi(argv[2]);
    cellsToDisplay = dims[0] * dims[1];
  }
  if (argc == 4)
  {
    cellsToDisplay = atoi(argv[3]);
  }
  numberOfInPoints = (dims[0] + 1) * (dims[1] + 1);

  // Create the input uniform cell set
  vtkm::cont::DataSet inDataSet = MakeTriangulateTestDataSet(dims);

  // Create the output dataset explicit cell set with same coordinate system
  vtkm::cont::CellSetSingleType<> cellSet(vtkm::CellShapeTagTriangle(), "cells");
  tetDataSet.AddCellSet(cellSet);
  tetDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(0));

  // Convert uniform hexahedra to tetrahedra
  tetrahedralizeFilter = new vtkm::worklet::TetrahedralizeFilterUniformGrid<DeviceAdapter>
                                              (inDataSet, tetDataSet);
  tetrahedralizeFilter->Run();

  // Render the output dataset of tets
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
  glutInitWindowSize(1000, 1000);
  glutInitWindowPosition(100, 100);

  glutCreateWindow("VTK-m Uniform Triangulate");

  initializeGL();

  glutDisplayFunc(displayCall);
  glutMainLoop();

  return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
# pragma GCC diagnostic pop
#endif
