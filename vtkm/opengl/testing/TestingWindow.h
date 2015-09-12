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
