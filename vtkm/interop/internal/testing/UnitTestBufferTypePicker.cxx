//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/cont/testing/Testing.h>

#include <GL/glew.h>
#include <vtkm/interop/internal/BufferTypePicker.h>

namespace
{
void TestBufferTypePicker()
{
  //just verify that certain types match
  GLenum type;
  using vtkmUint = unsigned int;
  using T = vtkm::FloatDefault;

  type = vtkm::interop::internal::BufferTypePicker(vtkm::Id());
  VTKM_TEST_ASSERT(type == GL_ELEMENT_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = vtkm::interop::internal::BufferTypePicker(int());
  VTKM_TEST_ASSERT(type == GL_ELEMENT_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = vtkm::interop::internal::BufferTypePicker(vtkmUint());
  VTKM_TEST_ASSERT(type == GL_ELEMENT_ARRAY_BUFFER, "Bad OpenGL Buffer Type");

  type = vtkm::interop::internal::BufferTypePicker(vtkm::Vec<T, 4>());
  VTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = vtkm::interop::internal::BufferTypePicker(vtkm::Vec<T, 3>());
  VTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = vtkm::interop::internal::BufferTypePicker(vtkm::FloatDefault());
  VTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = vtkm::interop::internal::BufferTypePicker(float());
  VTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = vtkm::interop::internal::BufferTypePicker(double());
  VTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
}
}

int UnitTestBufferTypePicker(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestBufferTypePicker);
}
