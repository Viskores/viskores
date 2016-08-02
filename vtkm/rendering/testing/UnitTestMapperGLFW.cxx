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
#include <GL/glew.h>
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

    typedef vtkm::rendering::MapperGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> M;
    typedef vtkm::rendering::CanvasGL C;
    typedef vtkm::rendering::View3D V3;
    typedef vtkm::rendering::View2D V2;    
    
    vtkm::cont::testing::MakeTestDataSet maker;
    vtkm::rendering::ColorTable colorTable("thermal");
    
    glfwInit();
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "GLFW Test", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glewInit();

    while (!glfwWindowShouldClose(window) && !done)
    {
       glfwPollEvents();

       if (which == 0)
           vtkm::rendering::testing::Render<M,C,V3>(maker.Make3DRegularDataSet0(),
                                                    "pointvar", colorTable, "reg3D.pnm");
       else if (which == 1)
           vtkm::rendering::testing::Render<M,C,V3>(maker.Make3DRectilinearDataSet0(),
                                                    "pointvar", colorTable, "rect3D.pnm");
       else if (which == 2)
           vtkm::rendering::testing::Render<M,C,V3>(maker.Make3DExplicitDataSet4(),
                                                    "pointvar", colorTable, "expl3D.pnm");
       else if (which == 3)
           vtkm::rendering::testing::Render<M,C,V2>(maker.Make2DRectilinearDataSet0(),
                                                    "pointvar", colorTable, "rect2D.pnm");       
       glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
}
} //namespace

int UnitTestMapperGLFW(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(RenderTests);
}
