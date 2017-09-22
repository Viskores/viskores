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

namespace detail
{
template <typename ArrayT>
struct PrepareForInteropFunctor
{
  ArrayT& Array;
  PrepareForInteropFunctor(ArrayT& array)
    : Array(array)
  {
  }

  template <typename DeviceAdapterTag>
  bool operator()(DeviceAdapterTag) const
  {
    using Traits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
    this->Array.PrepareForInput(DeviceAdapterTag());
    return this->Array.GetDeviceAdapterId() == Traits::GetId();
  }
};
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
template <typename ValueType, class StorageTag, class DeviceAdapterTag>
VTKM_CONT void TransferToOpenGL(vtkm::cont::ArrayHandle<ValueType, StorageTag> handle,
                                BufferState& state,
                                DeviceAdapterTag)
{
  vtkm::interop::internal::TransferToOpenGL<ValueType, DeviceAdapterTag> toGL(state);
  return toGL.Transfer(handle);
}

/// Dispatch overload for TransferToOpenGL that deduces the DeviceAdapter for
/// the given ArrayHandle.
///
/// \overload
///
template <typename ValueType, class StorageTag>
VTKM_CONT void TransferToOpenGL(vtkm::cont::ArrayHandle<ValueType, StorageTag> handle,
                                BufferState& state)
{
  vtkm::cont::DeviceAdapterId devId = handle.GetDeviceAdapterId();

  if (devId == VTKM_DEVICE_ADAPTER_UNDEFINED)
  {
    using ArrayT = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
    using Functor = detail::PrepareForInteropFunctor<ArrayT>;
    // Undefined device means that the array is not in an execution environment.
    // In this case, call PrepareForInput using the devices in the tracker
    // to move the data onto a device. This is required for a CUDA usecase
    // where a device pointer is set as control memory to reuse an already
    // allocated buffer. PrepareForInput on CUDA will detect this and set the
    // execution pointer to match the control pointer.
    vtkm::cont::TryExecute(Functor(handle));
    devId = handle.GetDeviceAdapterId();
  }

  switch (devId)
  {
    case VTKM_DEVICE_ADAPTER_SERIAL:
      TransferToOpenGL(handle, state, vtkm::cont::DeviceAdapterTagSerial());
      break;

#ifdef VTKM_ENABLE_TBB
    case VTKM_DEVICE_ADAPTER_TBB:
      TransferToOpenGL(handle, state, vtkm::cont::DeviceAdapterTagTBB());
      break;
#endif

#ifdef VTKM_CUDA
    case VTKM_DEVICE_ADAPTER_CUDA:
      TransferToOpenGL(handle, state, vtkm::cont::DeviceAdapterTagCuda());
      break;
#endif

    default:
      throw vtkm::cont::ErrorBadValue("Unknown device id.");
  }
}
}
}

#endif //vtk_m_interop_TransferToOpenGL_h
