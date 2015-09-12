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
#include <vtkm/opengl/internal/BufferTypePicker.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{
void TestBufferTypePicker()
{
  //just verify that certain types match
  GLenum type;
  typedef unsigned int vtkmUint;
  typedef vtkm::FloatDefault T;

  type = vtkm::opengl::internal::BufferTypePicker(vtkm::Id());
  VTKM_TEST_ASSERT(type == GL_ELEMENT_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = vtkm::opengl::internal::BufferTypePicker(int());
  VTKM_TEST_ASSERT(type == GL_ELEMENT_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = vtkm::opengl::internal::BufferTypePicker(vtkmUint());
  VTKM_TEST_ASSERT(type == GL_ELEMENT_ARRAY_BUFFER, "Bad OpenGL Buffer Type");

  type = vtkm::opengl::internal::BufferTypePicker(vtkm::Vec<T,4>());
  VTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = vtkm::opengl::internal::BufferTypePicker(vtkm::Vec<T,3>());
  VTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = vtkm::opengl::internal::BufferTypePicker(vtkm::FloatDefault());
  VTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = vtkm::opengl::internal::BufferTypePicker(float());
  VTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = vtkm::opengl::internal::BufferTypePicker(double());
  VTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
}
}


int UnitTestBufferTypePicker(int, char *[])
{
 return vtkm::cont::testing::Testing::Run(TestBufferTypePicker);
}
