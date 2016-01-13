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
#ifndef vtkm_m_opengl_testing_WindowBase_h
#define vtkm_m_opengl_testing_WindowBase_h

//constructs a valid openGL context so that we can verify
//that vtkm to open gl bindings work
#include <string>

#include <vtkm/internal/Configure.h>
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

// OpenGL Graphics includes
//glew needs to go before glut
#include <vtkm/opengl/internal/OpenGLHeaders.h>
#if defined (__APPLE__)
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
#endif

#include <vtkm/cont/ErrorControlBadValue.h>

#ifdef VTKM_CUDA
# include <vtkm/cont/cuda/ChooseCudaDevice.h>
# include <vtkm/opengl/cuda/SetOpenGLDevice.h>
#endif

#include <iostream>


namespace vtkm{
namespace opengl{
namespace testing{

namespace internal
{
template <typename T>
struct GLUTStaticCallbackHolder
{ static T* StaticGLUTResource; };

template <typename T>
T* GLUTStaticCallbackHolder<T>::StaticGLUTResource;

}


/// \brief Basic GLUT Wrapper class
///
/// This class gives the ability to wrap the glut function callbacks into
/// a single class so that you can use c++ objects. The only downside
/// is that you can only have a single window created
///
template< class Derived >
class WindowBase : private internal::GLUTStaticCallbackHolder<Derived>
{

public:
  void Init(std::string title, int width, int height,
            int argc, char** argv)
  {
  //set our selves as the static instance to call
  WindowBase<Derived>::StaticGLUTResource = static_cast<Derived*>(this);

  glutInit(&argc,argv);
  glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(width,height);
  glutCreateWindow(title.c_str());

  // glewExperimental = GL_TRUE;
  glewInit();

  if(!glewIsSupported("GL_VERSION_2_1"))
    {
    std::cerr << glGetString(GL_RENDERER) << std::endl;
    std::cerr << glGetString(GL_VERSION) << std::endl;
    throw vtkm::cont::ErrorControlBadValue(
                                  "Unable to create an OpenGL 2.1 Context");
    }

#ifdef VTKM_CUDA
    int id = vtkm::cuda::cont::FindFastestDeviceId();
    vtkm::opengl::cuda::SetCudaGLDevice(id);
#endif

  //attach all the glut call backs
  glutDisplayFunc( WindowBase<Derived>::GLUTDisplayCallback );
  glutIdleFunc( WindowBase<Derived>::GLUTIdleCallback );
  glutReshapeFunc( WindowBase<Derived>::GLUTChangeSizeCallback );
  glutKeyboardFunc( WindowBase<Derived>::GLUTKeyCallback );
  glutSpecialFunc( WindowBase<Derived>::GLUTSpecialKeyCallback );
  glutMouseFunc( WindowBase<Derived>::GLUTMouseCallback );
  glutMotionFunc( WindowBase<Derived>::GLUTMouseMoveCallback );
  glutPassiveMotionFunc( WindowBase<Derived>::GLUTPassiveMouseMoveCallback );

  //call any custom init code you want to have
  WindowBase<Derived>::StaticGLUTResource->PostInit();
  }

  void Init(std::string title, int width, int height)
  {
    int argc=0;
    char** argv = 0;
    Init(title,width,height,argc,argv);
  }

  //Init must be called before you call Start so that we have a valid
  //opengl context
  void Start()
  {
    glutMainLoop();
  }


  static void GLUTDisplayCallback()
    { WindowBase<Derived>::StaticGLUTResource->Display(); }

  static void GLUTIdleCallback()
    { WindowBase<Derived>::StaticGLUTResource->Idle(); }

  static void GLUTChangeSizeCallback(int width, int height)
    { WindowBase<Derived>::StaticGLUTResource->ChangeSize(width,height); }

  static void GLUTKeyCallback(unsigned char key, int x, int y)
    { WindowBase<Derived>::StaticGLUTResource->Key(key,x,y); }

  static void GLUTSpecialKeyCallback(int key, int x, int y)
    { WindowBase<Derived>::StaticGLUTResource->SpecialKey(key,x,y); }

  static void GLUTMouseCallback(int button, int state ,int x, int y)
    { WindowBase<Derived>::StaticGLUTResource->Mouse(button,state,x,y); }

  static void GLUTMouseMoveCallback(int x, int y)
    { WindowBase<Derived>::StaticGLUTResource->MouseMove(x,y); }

  static void GLUTPassiveMouseMoveCallback(int x, int y)
    { WindowBase<Derived>::StaticGLUTResource->PassiveMouseMove(x,y); }

};


}
}
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
# pragma GCC diagnostic pop
#endif

#endif //vtkm_m_opengl_testing_WindowBase_h
