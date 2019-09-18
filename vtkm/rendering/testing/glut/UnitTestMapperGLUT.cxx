//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/Bounds.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <GL/glew.h>
#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <string.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasGL.h>
#include <vtkm/rendering/MapperGL.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{
static constexpr vtkm::Id WIDTH = 512, HEIGHT = 512;
static vtkm::Id windowID, which = 0, NUM_DATASETS = 4;
static bool done = false;
static bool batch = false;

static void keyboardCall(unsigned char key, int vtkmNotUsed(x), int vtkmNotUsed(y))
{
  if (key == 27)
    glutDestroyWindow(windowID);
  else
  {
    which = (which + 1) % NUM_DATASETS;
    glutPostRedisplay();
  }
}

static void displayCall()
{
  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::ColorTable colorTable("inferno");

  using M = vtkm::rendering::MapperGL;
  using C = vtkm::rendering::CanvasGL;
  using V3 = vtkm::rendering::View3D;
  using V2 = vtkm::rendering::View2D;

  if (which == 0)
    vtkm::rendering::testing::Render<M, C, V3>(
      maker.Make3DRegularDataSet0(), "pointvar", colorTable, "reg3D.pnm");
  else if (which == 1)
    vtkm::rendering::testing::Render<M, C, V3>(
      maker.Make3DRectilinearDataSet0(), "pointvar", colorTable, "rect3D.pnm");
  else if (which == 2)
    vtkm::rendering::testing::Render<M, C, V3>(
      maker.Make3DExplicitDataSet4(), "pointvar", colorTable, "expl3D.pnm");
  else if (which == 3)
    vtkm::rendering::testing::Render<M, C, V2>(
      maker.Make2DRectilinearDataSet0(), "pointvar", colorTable, "rect2D.pnm");
  glutSwapBuffers();
}

void batchIdle()
{
  which++;
  if (which >= NUM_DATASETS)
    glutDestroyWindow(windowID);
  else
    glutPostRedisplay();
}

void RenderTests()
{
  if (!batch)
    std::cout << "Press any key to cycle through datasets. ESC to quit." << std::endl;

  int argc = 0;
  char* argv = nullptr;
  glutInit(&argc, &argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(WIDTH, HEIGHT);
  windowID = glutCreateWindow("GLUT test");
  glutDisplayFunc(displayCall);
  glutKeyboardFunc(keyboardCall);
  if (batch)
    glutIdleFunc(batchIdle);

  glutMainLoop();
}

} //namespace

int UnitTestMapperGLUT(int argc, char* argv[])
{
  if (argc > 1)
  {
    if (strcmp(argv[1], "-B") == 0)
    {
      batch = true;
    }
  }
  return vtkm::cont::testing::Testing::Run(RenderTests);
}
