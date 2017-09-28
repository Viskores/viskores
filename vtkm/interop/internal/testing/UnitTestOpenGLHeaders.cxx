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

int UnitTestOpenGLHeaders(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestOpenGLHeaders);
}
