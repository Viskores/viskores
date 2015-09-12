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
#ifndef vtk_m_opengl_internal_BufferTypePicker_h
#define vtk_m_opengl_internal_BufferTypePicker_h

#include <vtkm/Types.h>
#include <vtkm/opengl/internal/OpenGLHeaders.h>

namespace vtkm {
namespace opengl {
namespace internal {

/// helper function that guesses what OpenGL buffer type is the best default
/// given a primitive type. Currently GL_ELEMENT_ARRAY_BUFFER is used for integer
/// types, and GL_ARRAY_BUFFER is used for everything else
VTKM_CONT_EXPORT GLenum BufferTypePicker( int )
{ return GL_ELEMENT_ARRAY_BUFFER; }

VTKM_CONT_EXPORT GLenum BufferTypePicker( unsigned int )
{ return GL_ELEMENT_ARRAY_BUFFER; }

#if VTKM_SIZE_LONG == 8

VTKM_CONT_EXPORT GLenum BufferTypePicker( vtkm::Int64 )
{ return GL_ELEMENT_ARRAY_BUFFER; }

VTKM_CONT_EXPORT GLenum BufferTypePicker( vtkm::UInt64 )
{ return GL_ELEMENT_ARRAY_BUFFER; }

#endif

template<typename T>
VTKM_CONT_EXPORT GLenum BufferTypePicker( T )
{ return GL_ARRAY_BUFFER; }


}
}
} //namespace vtkm::opengl::internal

#endif //vtk_m_opengl_internal_BufferTypePicker_h
