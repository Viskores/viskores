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

//We first check if VTKM_DEVICE_ADAPTER is defined, so that when TBB and CUDA
//includes this file we use the device adapter that they have set.
#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif

#include <iostream>

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/opengl/TransferToOpenGL.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

//OpenGL Graphics includes
//glew needs to go before glut
//that is why this is after the TransferToOpenGL include
#if defined (__APPLE__)
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
#endif

#include "LoadShaders.h"

template< typename DeviceAdapter, typename T >
struct HelloVTKMInterop
{
  vtkm::Vec< vtkm::Int32, 2 > Dims;

  GLuint ProgramId;
  GLuint VBOId;
  GLuint VAOId;
  GLuint ColorId;

  vtkm::cont::Timer<DeviceAdapter> Timer;

  std::vector< vtkm::Vec< T, 3 > > InputData;
  vtkm::cont::ArrayHandle< vtkm::Vec< T, 3 > > InHandle;
  vtkm::cont::ArrayHandle< vtkm::Vec< T, 3 > > OutCoords;
  vtkm::cont::ArrayHandle< vtkm::Vec< vtkm::UInt8, 4 > > OutColors;

  HelloVTKMInterop(vtkm::Int32 width, vtkm::Int32 height):
    Dims(256,256),
    ProgramId(),
    VBOId(),
    VAOId(),
    ColorId(),
    Timer(),
    InputData(),
    InHandle(),
    OutCoords(),
    OutColors()
  {
    int dim = 256;
    this->InputData.reserve( static_cast<std::size_t>(dim*dim) );
    for (int i = 0; i < dim; ++i )
    {
      for (int j = 0; j < dim; ++j )
      {
      this->InputData.push_back( vtkm::Vec<T,3>( 2.f * i / dim - 1.f,
                                        0.f,
                                        2.f * j / dim - 1.f ) );
      }
    }

    this->Dims = vtkm::Vec< vtkm::Int32, 2 >( dim, dim );
    this->InHandle = vtkm::cont::make_ArrayHandle(this->InputData);

    glGenVertexArrays( 1, &this->VAOId );
    glBindVertexArray( this->VAOId );

    this->ProgramId = LoadShaders();
    glUseProgram( this->ProgramId );

    glClearColor( .08f, .08f, .08f, 0.f );
    glViewport(0, 0, width, height );
  }

  void render()
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    vtkm::Int32 arraySize = this->Dims[0]*this->Dims[1];

    //precomputed based on 1027x768 render window size
    vtkm::Float32 mvp[16] = {-1.79259f, 0.f, 0.f, 0.f,
                              0.f, 1.26755f, -0.721392f, -0.707107f,
                              0.f, 1.26755f, 0.721392f, 0.707107f,
                              0.f, 0.f, 1.24076f, 1.41421f};

    GLint unifLoc = glGetUniformLocation( this->ProgramId, "MVP");
    glUniformMatrix4fv( unifLoc, 1, GL_FALSE, mvp );

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBOId);
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, 0 );

    glEnableClientState(GL_COLOR_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, this->ColorId);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0 );

    glDrawArrays( GL_POINTS, 0, arraySize );

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableVertexAttribArray(0);
  }

  struct GenerateSurfaceWorklet : public vtkm::worklet::WorkletMapField
  {
    vtkm::Float32 t;
    GenerateSurfaceWorklet(vtkm::Float32 st) : t(st) {}

    typedef void ControlSignature( FieldIn<>, FieldOut<>, FieldOut<> );
    typedef void ExecutionSignature( _1, _2, _3 );

    VTKM_EXEC_EXPORT
    void operator()( const vtkm::Vec< T, 3 > & input,
                     vtkm::Vec<T, 3> & output,
                     vtkm::Vec<vtkm::UInt8, 4>& color ) const
    {
      output[0] = input[0];
      output[1] = 0.25f * vtkm::Sin( input[0] * 10.f + t ) * vtkm::Cos( input[2] * 10.f + t );
      output[2] = input[2];

      color[0] = 0;
      color[1] = 160 + static_cast<vtkm::UInt8>(96 * vtkm::Sin( input[0] * 10.f + t ) );
      color[2] = 160 + static_cast<vtkm::UInt8>(96 * vtkm::Cos( input[2] * 5.f + t ) );
      color[3] = 255;
    }
  };

  void renderFrame(  )
  {
  typedef vtkm::worklet::DispatcherMapField<GenerateSurfaceWorklet> DispatcherType;

  vtkm::Float32 t = static_cast<vtkm::Float32>(this->Timer.GetElapsedTime());

  GenerateSurfaceWorklet worklet( t );
  DispatcherType(worklet).Invoke( this->InHandle, this->OutCoords, this->OutColors );

  vtkm::opengl::TransferToOpenGL( this->OutCoords, this->VBOId, DeviceAdapter() );
  vtkm::opengl::TransferToOpenGL( this->OutColors, this->ColorId, DeviceAdapter() );

  this->render();
  if(t > 10)
  {
    //after 10seconds quit the demo
    exit(0);
  }
  }
};

//global static so that glut callback can access it
typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;
HelloVTKMInterop< DeviceAdapter, vtkm::Float32 >* helloWorld = NULL;

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
  typedef vtkm::cont::internal::DeviceAdapterTraits<DeviceAdapter>
                                                        DeviceAdapterTraits;
  std::cout << "Running Hello World example on device adapter: "
            << DeviceAdapterTraits::GetId() << std::endl;

  glewExperimental = GL_TRUE;
  glutInit(&argc, argv);

  const vtkm::UInt32 width  = 1024;
  const vtkm::UInt32 height = 768;

  glutInitWindowSize ( width, height );
  glutInitDisplayMode ( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("VTK-m Hello World OpenGL Interop");

	GLenum err = glewInit();
  if (GLEW_OK != err)
  {
  	std::cout << "glewInit failed\n";
  }

  HelloVTKMInterop< DeviceAdapter, vtkm::Float32 > hw(width,height);
  helloWorld = &hw;

  glutDisplayFunc(run);
  glutIdleFunc(idle);
  glutMainLoop();
}


