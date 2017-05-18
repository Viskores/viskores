//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/Bounds.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasGL.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/MapperGL.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View2D.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

// this needs to be included after the vtk-m headers so that we include
// the gl headers in the correct order
#include <GLFW/glfw3.h>

#include <cstring>
#include <string>

namespace
{
static const vtkm::Id WIDTH = 512, HEIGHT = 512;
static vtkm::Id which = 0, NUM_DATASETS = 4;
static bool done = false;
static bool batch = false;

static void keyCallback(GLFWwindow* vtkmNotUsed(window), int key, int vtkmNotUsed(scancode),
                        int action, int vtkmNotUsed(mods))
{
  if (key == GLFW_KEY_ESCAPE)
    done = true;
  if (action == 1)
    which = (which + 1) % NUM_DATASETS;
}

void RenderTests()
{
  std::cout << "Press any key to cycle through datasets. ESC to quit." << std::endl;

  typedef vtkm::rendering::MapperGL MapperType;
  typedef vtkm::rendering::CanvasGL CanvasType;
  typedef vtkm::rendering::View3D View3DType;
  typedef vtkm::rendering::View2D View2DType;

  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::rendering::ColorTable colorTable("thermal");

  glfwInit();
  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "GLFW Test", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, keyCallback);

  CanvasType canvas[4] = { CanvasType(512, 512), CanvasType(512, 512), CanvasType(512, 512),
                           CanvasType(512, 512) };
  vtkm::rendering::Scene scene[4];
  vtkm::cont::DataSet ds[4];
  MapperType mapper[4];
  vtkm::rendering::Camera camera[4];

  ds[0] = maker.Make3DRegularDataSet0();
  ds[1] = maker.Make3DRectilinearDataSet0();
  ds[2] = maker.Make3DExplicitDataSet4();
  ds[3] = maker.Make2DRectilinearDataSet0();

  std::string fldNames[4];
  fldNames[0] = "pointvar";
  fldNames[1] = "pointvar";
  fldNames[2] = "pointvar";
  fldNames[3] = "pointvar";

  for (int i = 0; i < NUM_DATASETS; i++)
  {
    scene[i].AddActor(vtkm::rendering::Actor(ds[i].GetCellSet(), ds[i].GetCoordinateSystem(),
                                             ds[i].GetField(fldNames[i].c_str()), colorTable));
    vtkm::rendering::testing::SetCamera<View3DType>(camera[i],
                                                    ds[i].GetCoordinateSystem().GetBounds());
  }

  View3DType view3d0(scene[0], mapper[0], canvas[0], camera[0],
                     vtkm::rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));
  View3DType view3d1(scene[1], mapper[1], canvas[1], camera[1],
                     vtkm::rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));
  View3DType view3d2(scene[2], mapper[2], canvas[2], camera[2],
                     vtkm::rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));

  View2DType view2d(scene[3], mapper[3], canvas[3], camera[3],
                    vtkm::rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));

  while (!glfwWindowShouldClose(window) && !done)
  {
    glfwPollEvents();

    if (which == 0)
      vtkm::rendering::testing::Render<MapperType, CanvasType, View3DType>(view3d0, "reg3D.pnm");
    else if (which == 1)
      vtkm::rendering::testing::Render<MapperType, CanvasType, View3DType>(view3d1, "rect3D.pnm");
    else if (which == 2)
      vtkm::rendering::testing::Render<MapperType, CanvasType, View3DType>(view3d2, "expl3D.pnm");
    else if (which == 3)
      vtkm::rendering::testing::Render<MapperType, CanvasType, View2DType>(view2d, "rect2D.pnm");
    glfwSwapBuffers(window);

    if (batch)
    {
      which++;
      if (which >= NUM_DATASETS)
      {
        break;
      }
    }
  }

  glfwDestroyWindow(window);
}
} //namespace

int UnitTestMapperGLFW(int argc, char* argv[])
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
