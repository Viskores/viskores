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
#ifndef vtk_m_opengl_TransferToOpenGL_h
#define vtk_m_opengl_TransferToOpenGL_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/opengl/internal/TransferToOpenGL.h>

namespace vtkm{
namespace opengl{
/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible. Will return the type of array buffer
/// that we have bound the handle too. Will be GL_ELEMENT_ARRAY_BUFFER for
/// vtkm::Id, and GL_ARRAY_BUFFER for everything else.
///
/// This function keeps the buffer as the active buffer of the returned type.
///
/// This function will throw exceptions if the transfer wasn't possible
///
template<typename ValueType, class StorageTag, class DeviceAdapterTag>
VTKM_CONT_EXPORT
GLenum TransferToOpenGL(vtkm::cont::ArrayHandle<ValueType,StorageTag> handle,
                        GLuint& openGLHandle,
                        DeviceAdapterTag)
{
  vtkm::opengl::internal::TransferToOpenGL<ValueType, DeviceAdapterTag> toGL;
  toGL.Transfer(handle,openGLHandle);
  return toGL.GetType();
}

/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible. Will use the given \p type as how
/// to bind the buffer.
///
/// This function keeps the buffer as the active buffer of the input type.
///
/// This function will throw exceptions if the transfer wasn't possible
///
template<typename ValueType, class StorageTag, class DeviceAdapterTag>
VTKM_CONT_EXPORT
void TransferToOpenGL(vtkm::cont::ArrayHandle<ValueType, StorageTag> handle,
                      GLuint& openGLHandle,
                      GLenum type,
                      DeviceAdapterTag)
{
  vtkm::opengl::internal::TransferToOpenGL<ValueType, DeviceAdapterTag> toGL(type);
  toGL.Transfer(handle,openGLHandle);
}

}}

#endif //vtk_m_opengl_TransferToOpenGL_h
