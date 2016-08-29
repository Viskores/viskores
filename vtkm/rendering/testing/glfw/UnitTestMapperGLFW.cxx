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
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <GLFW/glfw3.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasGL.h>
#include <vtkm/rendering/MapperGL.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace {
static const vtkm::Id WIDTH = 512, HEIGHT = 512;
static vtkm::Id which = 0, NUM_DATASETS = 4;
static bool done = false;
static bool batch = false;

static void
keyCallback(GLFWwindow* vtkmNotUsed(window), int key,
            int vtkmNotUsed(scancode), int action, int vtkmNotUsed(mods))
{
  if (key == GLFW_KEY_ESCAPE)
      done = true;
  if (action == 1)
    which = (which+1) % NUM_DATASETS;
}   

void RenderTests()
{
    std::cout<<"Press any key to cycle through datasets. ESC to quit."<<std::endl;

    typedef vtkm::rendering::MapperGL MapperType;
    typedef vtkm::rendering::CanvasGL CanvasType;
    typedef vtkm::rendering::View3D View3DType;
    typedef vtkm::rendering::View2D View2DType;
    
    vtkm::cont::testing::MakeTestDataSet maker;
    vtkm::rendering::ColorTable colorTable("thermal");
    
    glfwInit();
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "GLFW Test", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    
    while (!glfwWindowShouldClose(window) && !done)
    {
       glfwPollEvents();

       if (which == 0)
           vtkm::rendering::testing::Render<MapperType,CanvasType,View3DType>(maker.Make3DRegularDataSet0(),
                                                    "pointvar", colorTable, "reg3D.pnm");
       else if (which == 1)
           vtkm::rendering::testing::Render<MapperType,CanvasType,View3DType>(maker.Make3DRectilinearDataSet0(),
                                                    "pointvar", colorTable, "rect3D.pnm");
       else if (which == 2)
           vtkm::rendering::testing::Render<MapperType,CanvasType,View3DType>(maker.Make3DExplicitDataSet4(),
                                                    "pointvar", colorTable, "expl3D.pnm");
       else if (which == 3)
           vtkm::rendering::testing::Render<MapperType,CanvasType,View2DType>(maker.Make2DRectilinearDataSet0(),
                                                    "pointvar", colorTable, "rect2D.pnm");       
       glfwSwapBuffers(window);

       if (batch)
       {
         which++;
         if (which >= NUM_DATASETS) { break; }
       }
    }

    glfwDestroyWindow(window);
}
} //namespace

int UnitTestMapperGLFW(int argc, char *argv[])
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
