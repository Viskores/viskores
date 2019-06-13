//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/filter/Triangulate.h>

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

namespace
{

// Takes input uniform grid and outputs unstructured grid of triangles
static vtkm::cont::DataSet outDataSet;

} // anonymous namespace

//
// Construct an input data set with uniform grid of indicated dimensions, origin and spacing
//
vtkm::cont::DataSet MakeTriangulateExplicitDataSet()
{
  vtkm::cont::DataSetBuilderExplicitIterative builder;
  builder.Begin();

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
  vtkm::cont::CellSetSingleType<> cellSet;
  outDataSet.GetCellSet(0).CopyTo(cellSet);
  vtkm::Id numberOfCells = cellSet.GetNumberOfCells();

  auto vertexArray = outDataSet.GetCoordinateSystem().GetData();

  // Draw the two triangles belonging to each quad
  vtkm::Float32 color[4][3] = {
    { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 0.0f }
  };

  for (vtkm::Id triangle = 0; triangle < numberOfCells; triangle++)
  {
    vtkm::Id indx = triangle % 4;
    glColor3f(color[indx][0], color[indx][1], color[indx][2]);

    // Get the indices of the vertices that make up this triangle
    vtkm::Vec<vtkm::Id, 3> triIndices;
    cellSet.GetIndices(triangle, triIndices);

    // Get the vertex points for this triangle
    vtkm::Vec<vtkm::Float64, 3> pt0 = vertexArray.GetPortalConstControl().Get(triIndices[0]);
    vtkm::Vec<vtkm::Float64, 3> pt1 = vertexArray.GetPortalConstControl().Get(triIndices[1]);
    vtkm::Vec<vtkm::Float64, 3> pt2 = vertexArray.GetPortalConstControl().Get(triIndices[2]);

    // Draw the triangle filled with alternating colors
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
  auto opts =
    vtkm::cont::InitializeOptions::DefaultAnyDevice | vtkm::cont::InitializeOptions::Strict;
  vtkm::cont::Initialize(argc, argv, opts);
  std::cout << "TrianguleExplicitGrid Example" << std::endl;

  // Create the input uniform cell set
  vtkm::cont::DataSet inDataSet = MakeTriangulateExplicitDataSet();
  vtkm::cont::CellSetExplicit<> inCellSet;
  inDataSet.GetCellSet(0).CopyTo(inCellSet);

  // Convert 2D explicit cells to triangles
  vtkm::filter::Triangulate triangulate;
  outDataSet = triangulate.Execute(inDataSet);

  // Render the output dataset of tets
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
  glutInitWindowSize(1000, 1000);
  glutInitWindowPosition(100, 100);

  glutCreateWindow("VTK-m Explicit Triangulate");

  initializeGL();

  glutDisplayFunc(displayCall);
  glutMainLoop();

  outDataSet.Clear();
  return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif
