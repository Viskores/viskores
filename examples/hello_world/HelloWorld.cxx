//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// Must be included before any other GL includes:
#include <GL/glew.h>

#include <iostream>

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/interop/TransferToOpenGL.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

//Suppress warnings about glut being deprecated on OSX
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

//OpenGL Graphics includes
//glew needs to go before glut
//that is why this is after the TransferToOpenGL include
#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "LoadShaders.h"

template <typename T>
struct HelloVTKMInterop
{
  vtkm::Vec<vtkm::Int32, 2> Dims;

  GLuint ProgramId;
  GLuint VAOId;

  vtkm::interop::BufferState VBOState;
  vtkm::interop::BufferState ColorState;

  vtkm::cont::Timer Timer;

  std::vector<vtkm::Vec<T, 3>> InputData;
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> InHandle;
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> OutCoords;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>> OutColors;

  HelloVTKMInterop(vtkm::Int32 width, vtkm::Int32 height)
    : Dims(256, 256)
    , ProgramId()
    , VAOId()
    , VBOState()
    , ColorState()
    , Timer()
    , InputData()
    , InHandle()
    , OutCoords()
    , OutColors()
  {
    Timer.Start();
    int dim = 256;
    this->InputData.reserve(static_cast<std::size_t>(dim * dim));
    for (int i = 0; i < dim; ++i)
    {
      for (int j = 0; j < dim; ++j)
      {
        vtkm::Vec<T, 3> t(2.f * (static_cast<T>(i) / static_cast<T>(dim)) - 1.f,
                          0.f,
                          2.f * (static_cast<T>(j) / static_cast<T>(dim)) - 1.f);
        this->InputData.push_back(t);
      }
    }

    this->Dims = vtkm::Vec<vtkm::Int32, 2>(dim, dim);
    this->InHandle = vtkm::cont::make_ArrayHandle(this->InputData);

    glGenVertexArrays(1, &this->VAOId);
    glBindVertexArray(this->VAOId);

    this->ProgramId = LoadShaders();
    glUseProgram(this->ProgramId);

    glClearColor(.08f, .08f, .08f, 0.f);
    glViewport(0, 0, width, height);
  }

  void render()
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    vtkm::Int32 arraySize = this->Dims[0] * this->Dims[1];

    //precomputed based on 1027x768 render window size
    vtkm::Float32 mvp[16] = { -1.79259f,  0.f,        0.f,      0.f,      0.f,       1.26755f,
                              -0.721392f, -0.707107f, 0.f,      1.26755f, 0.721392f, 0.707107f,
                              0.f,        0.f,        1.24076f, 1.41421f };

    GLint unifLoc = glGetUniformLocation(this->ProgramId, "MVP");
    glUniformMatrix4fv(unifLoc, 1, GL_FALSE, mvp);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, *this->VBOState.GetHandle());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableClientState(GL_COLOR_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, *this->ColorState.GetHandle());
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);

    glDrawArrays(GL_POINTS, 0, arraySize);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableVertexAttribArray(0);
  }

  struct GenerateSurfaceWorklet : public vtkm::worklet::WorkletMapField
  {
    vtkm::Float32 t;
    GenerateSurfaceWorklet(vtkm::Float32 st)
      : t(st)
    {
    }

    using ControlSignature = void(FieldIn, FieldOut, FieldOut);
    using ExecutionSignature = void(_1, _2, _3);

    VTKM_EXEC
    void operator()(const vtkm::Vec<T, 3>& input,
                    vtkm::Vec<T, 3>& output,
                    vtkm::Vec<vtkm::UInt8, 4>& color) const
    {
      output[0] = input[0];
      output[1] = 0.25f * vtkm::Sin(input[0] * 10.f + t) * vtkm::Cos(input[2] * 10.f + t);
      output[2] = input[2];

      color[0] = 0;
      color[1] = static_cast<vtkm::UInt8>(160 + 96 * vtkm::Sin(input[0] * 10.f + t));
      color[2] = static_cast<vtkm::UInt8>(160 + 96 * vtkm::Cos(input[2] * 5.f + t));
      color[3] = 255;
    }
  };

  void renderFrame()
  {
    using DispatcherType = vtkm::worklet::DispatcherMapField<GenerateSurfaceWorklet>;

    vtkm::Float32 t = static_cast<vtkm::Float32>(this->Timer.GetElapsedTime());

    GenerateSurfaceWorklet worklet(t);
    DispatcherType(worklet).Invoke(this->InHandle, this->OutCoords, this->OutColors);

    vtkm::interop::TransferToOpenGL(this->OutCoords, this->VBOState);
    vtkm::interop::TransferToOpenGL(this->OutColors, this->ColorState);
    this->render();
    if (t > 10)
    {
      //after 10seconds quit the demo
      exit(0);
    }
  }
};

//global static so that glut callback can access it
HelloVTKMInterop<vtkm::Float32>* helloWorld = nullptr;

// Render the output using simple OpenGL
void run()
{
  helloWorld->renderFrame();
  glutSwapBuffers();
}

void idle()
{
  glutPostRedisplay();
}

int main(int argc, char** argv)
{
  auto opts =
    vtkm::cont::InitializeOptions::DefaultAnyDevice | vtkm::cont::InitializeOptions::Strict;
  vtkm::cont::Initialize(argc, argv, opts);
  std::cout << "Running Hello World example: " << std::endl;

  glewExperimental = GL_TRUE;
  glutInit(&argc, argv);

  const vtkm::UInt32 width = 1024;
  const vtkm::UInt32 height = 768;

  glutInitWindowSize(width, height);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("VTK-m Hello World OpenGL Interop");

  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    std::cout << "glewInit failed\n";
  }

  HelloVTKMInterop<vtkm::Float32> hw(width, height);
  helloWorld = &hw;

  glutDisplayFunc(run);
  glutIdleFunc(idle);
  glutMainLoop();
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif
