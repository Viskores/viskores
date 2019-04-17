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
#include <vtkm/cont/DataSet.h>
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

// Default size of the example
static vtkm::Id2 dims(4, 4);
static vtkm::Id cellsToDisplay = 16;

// Takes input uniform grid and outputs unstructured grid of triangles
static vtkm::cont::DataSet triDataSet;

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
    1.0f / static_cast<vtkm::Float32>(dim[0]), 1.0f / static_cast<vtkm::Float32>(dim[1]), 0.0f);

  // Generate coordinate system
  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims, origin, spacing);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  // Generate cell set
  vtkm::cont::CellSetStructured<2> cellSet("cells");
  cellSet.SetPointDimensions(vtkm::make_Vec(dim[0] + 1, dim[1] + 1));
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

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
  triDataSet.GetCellSet(0).CopyTo(cellSet);

  auto vertexArray = triDataSet.GetCoordinateSystem().GetData();

  // Draw the two triangles belonging to each quad
  vtkm::Id triangle = 0;
  vtkm::Float32 color[4][3] = {
    { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 0.0f }
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

      triangle++;
    }
  }
  glFlush();
}

// Triangulate and render uniform grid example
int main(int argc, char* argv[])
{
  // Process vtk-m general args
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  auto config = vtkm::cont::Initialize(argc, argv, opts);

  std::cout << "TrianguleUniformGrid Example" << std::endl;
  std::cout << "Parameters are [options] [xdim ydim [# of cellsToDisplay]]" << std::endl;
  std::cout << config.Usage << std::endl << std::endl;
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

  // Create the input uniform cell set
  vtkm::cont::DataSet inDataSet = MakeTriangulateTestDataSet(dims);

  // Convert uniform quad to triangle
  vtkm::filter::Triangulate triangulate;
  triDataSet = triangulate.Execute(inDataSet);

  // Render the output dataset of tets
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
  glutInitWindowSize(1000, 1000);
  glutInitWindowPosition(100, 100);

  glutCreateWindow("VTK-m Uniform Triangulate");

  initializeGL();

  glutDisplayFunc(displayCall);
  glutMainLoop();

  triDataSet.Clear();
  return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif
