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
#ifndef vtk_m_interop_TransferToOpenGL_h
#define vtk_m_interop_TransferToOpenGL_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/interop/BufferState.h>
#include <vtkm/interop/internal/TransferToOpenGL.h>

namespace vtkm{
namespace interop{


/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible. Will use the given \p state to determine
/// what buffer handle to use, and the type to bind the buffer handle too.
/// If the type of buffer hasn't been determined the transfer will use
/// deduceAndSetBufferType to do so. Lastly state also holds on to per backend resources
/// that allow for efficient updating to open gl
///
/// This function keeps the buffer as the active buffer of the input type.
///
/// This function will throw exceptions if the transfer wasn't possible
///
template<typename ValueType, class StorageTag, class DeviceAdapterTag>
VTKM_CONT
void TransferToOpenGL(vtkm::cont::ArrayHandle<ValueType, StorageTag> handle,
                      BufferState& state,
                      DeviceAdapterTag)
{
  vtkm::interop::internal::TransferToOpenGL<ValueType, DeviceAdapterTag> toGL(state);
  return toGL.Transfer(handle);
}

}}

#endif //vtk_m_interop_TransferToOpenGL_h
