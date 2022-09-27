//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_interop_TransferToOpenGL_h
#define vtk_m_interop_TransferToOpenGL_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/interop/BufferState.h>
#include <vtkm/interop/internal/TransferToOpenGL.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

namespace vtkm
{
namespace interop
{

/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible. Will use the given \p state to determine
/// what buffer handle to use, and the type to bind the buffer handle too.
/// Lastly state also holds on to per backend resources that allow for efficient
/// updating to open gl.
///
/// This function keeps the buffer as the active buffer of the input type.
///
///
template <typename ValueType, class StorageTag, class DeviceAdapterTag>
VTKM_CONT void TransferToOpenGL(const vtkm::cont::ArrayHandle<ValueType, StorageTag>& handle,
                                BufferState& state,
                                DeviceAdapterTag)
{
  vtkm::interop::internal::TransferToOpenGL<ValueType, DeviceAdapterTag> toGL(state);
  toGL.Transfer(handle);
}

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
template <typename ValueType, typename StorageTag>
VTKM_CONT void TransferToOpenGL(const vtkm::cont::ArrayHandle<ValueType, StorageTag>& handle,
                                BufferState& state)
{
  // First, try to transfer data that already exists on a device.
  bool success = vtkm::cont::TryExecute([&](auto device) {
    if (handle.IsOnDevice(device))
    {
      TransferToOpenGL(handle, state, device);
      return true;
    }
    else
    {
      return false;
    }
  });
  if (!success)
  {
    // Generally, we are here because the array is not already on a device
    // or for some reason the transfer failed on that device. Try any device.
    success = vtkm::cont::TryExecute([&](auto device) {
      TransferToOpenGL(handle, state, device);
      return true;
    });
  }
  if (!success)
  {
    throw vtkm::cont::ErrorBadValue("Failed to transfer array to OpenGL on any device.");
  }
}
}
}

#endif //vtk_m_interop_TransferToOpenGL_h
