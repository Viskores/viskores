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

#include <vtkm/worklet/TetrahedralizeExplicitGrid.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/testing/Testing.h>

//Suppress warnings about glut being deprecated on OSX
#if (defined(VTKM_GCC) || defined(VTKM_CLANG)) && !defined(VTKM_PGI)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#if defined (__APPLE__)
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
#endif

typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

// Takes input uniform grid and outputs unstructured grid of triangles
vtkm::worklet::TetrahedralizeFilterExplicitGrid<DeviceAdapter> *tetrahedralizeFilter;
vtkm::cont::DataSet outDataSet;
vtkm::Id numberOfInPoints;

// Point location of vertices from a CastAndCall but needs a static cast eventually
vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3> > vertexArray;

//
// Construct an input data set with uniform grid of indicated dimensions, origin and spacing
//
vtkm::cont::DataSet MakeTriangulateExplicitDataSet()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 16;
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0),   // 0
    CoordType(1, 0, 0),   // 1
    CoordType(2, 0, 0),   // 2
    CoordType(3, 0, 0),   // 3
    CoordType(0, 1, 0),   // 4
    CoordType(1, 1, 0),   // 5
    CoordType(2, 1, 0),   // 6
    CoordType(3, 1, 0),   // 7
    CoordType(0, 2, 0),   // 8
    CoordType(1, 2, 0),   // 9
    CoordType(2, 2, 0),   // 10
    CoordType(3, 2, 0),   // 11
    CoordType(0, 3, 0),   // 12
    CoordType(3, 3, 0),   // 13
    CoordType(1, 4, 0),   // 14
    CoordType(2, 4, 0),   // 15
  };

  dataSet.AddCoordinateSystem(
          vtkm::cont::CoordinateSystem("coordinates", 1, coordinates, nVerts));

  std::vector<vtkm::UInt8> shapes;
  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  shapes.push_back(vtkm::CELL_SHAPE_POLYGON);

  std::vector<vtkm::IdComponent> numindices;
  numindices.push_back(3);
  numindices.push_back(4);
  numindices.push_back(4);
  numindices.push_back(4);
  numindices.push_back(3);
  numindices.push_back(4);
  numindices.push_back(6);

  std::vector<vtkm::Id> conn;
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(5);

  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(6);
  conn.push_back(5);

  conn.push_back(5);
  conn.push_back(6);
  conn.push_back(10);
  conn.push_back(9);

  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(9);
  conn.push_back(8);

  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(7);

  conn.push_back(6);
  conn.push_back(7);
  conn.push_back(11);
  conn.push_back(10);

  conn.push_back(9);
  conn.push_back(10);
  conn.push_back(13);
  conn.push_back(15);
  conn.push_back(14);
  conn.push_back(12);

  static const vtkm::IdComponent ndim = 2;
  vtkm::cont::CellSetExplicit<> cellSet(nVerts, "cells", ndim);
  cellSet.FillViaCopy(shapes, numindices, conn);

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

#if (defined(VTKM_GCC) || defined(VTKM_CLANG)) && !defined(VTKM_PGI)
# pragma GCC diagnostic pop
#endif
