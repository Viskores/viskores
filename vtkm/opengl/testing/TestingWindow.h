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
#ifndef vtk_m_opengl_testing_TestingWindow_h
#define vtk_m_opengl_testing_TestingWindow_h

#include <vtkm/internal/ExportMacros.h>
#include <vtkm/opengl/testing/WindowBase.h>

#if defined(VTKM_GCC) && defined(VTKM_POSIX) && !defined(__APPLE__)
//
// 1. Some Linux distributions default linker implicitly enables the as-needed
// linking flag. This means that your shared library or executable will only
// link to libraries from which they use symbols. So if you explicitly link to
// pthread but don't use any symbols you wont have a 'DT_NEEDED' entry for
// pthread.
//
// 2. NVidia libGL (driver version 352 ) uses pthread but doesn't have
// a DT_NEEDED entry for the library. When you run ldd or readelf on the library
// you won't detect any reference to the pthread library. Aside this is odd
// since the mesa version does explicitly link to pthread. But if you run the
// following command:
//        "strings  /usr/lib/nvidia-352/libGL.so.1 | grep pthread | less"
// You will see the following:
// { pthread_create
//   pthread_self
//   pthread_equal
//   pthread_key_crea
//   ...
//   libpthread.so.0
//   libpthread.so
//   pthread_create
// }
//
// This is very strong evidence that this library is using pthread.
//
//
// 3. So what does this all mean?
//
// It means that on system that use the linking flag 'as-needed', are using
// the nvidia driver, and don't use pthread will generate binaries that crash
// on launch. The only way to work around this issue is to do either:
//
//
//  A: Specify 'no-as-needed' to the linker potentially causing over-linking
//  and a  slow down in link time.
//
//  B: Use a method from pthread, making the linker realize that pthread is
//  needed. Note we have to actually call the method so that a linker with
//  optimizations enabled doesn't remove the function and pthread requirement.
//
//
// So that is the explanation on why we have the following function which is
// used once, doesn't look to be useful and seems very crazy.
#include <pthread.h>
#include <iostream>
#define VTKM_NVIDIA_PTHREAD_WORKAROUND 1
static int vtkm_force_linking_to_pthread_to_fix_nvidia_libgl_bug()
  { return static_cast<int>(pthread_self()); }
#endif


namespace vtkm{
namespace opengl{
namespace testing{

/// \brief Basic Render Window that only makes sure opengl has a valid context
///
/// Bare-bones class that fulfullis the requirements of WindowBase but
/// has no ability to interact with opengl other than to close down the window
///
///
class TestingWindow : public vtkm::opengl::testing::WindowBase<TestingWindow>
{
public:
  VTKM_CONT_EXPORT TestingWindow(){};

  //called after opengl is inited
  VTKM_CONT_EXPORT void PostInit()
  {}

  VTKM_CONT_EXPORT void Display()
  {}

  VTKM_CONT_EXPORT void Idle()
  {}

  VTKM_CONT_EXPORT void ChangeSize(int vtkmNotUsed(w), int vtkmNotUsed(h) )
  {}

  VTKM_CONT_EXPORT void Key(unsigned char key,
                           int vtkmNotUsed(x), int vtkmNotUsed(y) )
  {
   if(key == 27) //escape pressed
    {
#if defined(VTKM_NVIDIA_PTHREAD_WORKAROUND)
    std::cout << ::vtkm_force_linking_to_pthread_to_fix_nvidia_libgl_bug();
#endif
    exit(0);
    }
  }

  VTKM_CONT_EXPORT void SpecialKey(int vtkmNotUsed(key),
                                  int vtkmNotUsed(x), int vtkmNotUsed(y) )
  {}

  VTKM_CONT_EXPORT void Mouse(int vtkmNotUsed(button), int vtkmNotUsed(state),
                             int vtkmNotUsed(x), int vtkmNotUsed(y) )
  {}

  VTKM_CONT_EXPORT void MouseMove(int vtkmNotUsed(x), int vtkmNotUsed(y) )
  {}

  VTKM_CONT_EXPORT void PassiveMouseMove(int vtkmNotUsed(x), int vtkmNotUsed(y) )
  {}

};


}
}
}
#endif //vtk_m_opengl_testing_TestingWindow_h
