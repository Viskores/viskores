//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_interop_internal_BufferTypePicker_h
#define vtk_m_interop_internal_BufferTypePicker_h

#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/interop/internal/OpenGLHeaders.h>

namespace vtkm
{
namespace interop
{
namespace internal
{

namespace detail
{

template <typename NumericTag, typename DimensionalityTag>
static inline VTKM_CONT GLenum BufferTypePickerImpl(NumericTag, DimensionalityTag)
{
  return GL_ARRAY_BUFFER;
}

VTKM_CONT
static inline GLenum BufferTypePickerImpl(vtkm::TypeTraitsIntegerTag, vtkm::TypeTraitsScalarTag)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

} //namespace detail

static inline VTKM_CONT GLenum BufferTypePicker(vtkm::Int32)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

static inline VTKM_CONT GLenum BufferTypePicker(vtkm::UInt32)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

static inline VTKM_CONT GLenum BufferTypePicker(vtkm::Int64)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

static inline VTKM_CONT GLenum BufferTypePicker(vtkm::UInt64)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

/// helper function that guesses what OpenGL buffer type is the best default
/// given a primitive type. Currently GL_ELEMENT_ARRAY_BUFFER is used for
/// integer types, and GL_ARRAY_BUFFER is used for everything else
///
template <typename T>
static inline VTKM_CONT GLenum BufferTypePicker(T)
{
  using Traits = vtkm::TypeTraits<T>;
  return detail::BufferTypePickerImpl(typename Traits::NumericTag(),
                                      typename Traits::DimensionalityTag());
}
}
}
} //namespace vtkm::interop::internal

#endif //vtk_m_interop_internal_BufferTypePicker_h
