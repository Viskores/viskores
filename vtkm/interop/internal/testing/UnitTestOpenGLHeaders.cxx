//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <GL/glew.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/interop/internal/OpenGLHeaders.h>

namespace
{
void TestOpenGLHeaders()
{
#if defined(GL_VERSION_1_3) && (GL_VERSION_1_3 == 1)
  //this is pretty simple, we just verify that certain symbols exist
  //and the version of openGL is high enough that our interop will work.
  GLenum e = GL_ELEMENT_ARRAY_BUFFER;
  GLuint u = 1;
  u = u * e;
  (void)u;
#else
  unable_to_find_required_gl_version();
#endif
}
}

int UnitTestOpenGLHeaders(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestOpenGLHeaders, argc, argv);
}
