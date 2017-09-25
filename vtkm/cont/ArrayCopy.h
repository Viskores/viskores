//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ArrayCopy_h
#define vtk_m_cont_ArrayCopy_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/TryExecute.h>

// TODO: When virtual arrays are available, compile the implementation in a .cxx/.cu file. Common
// arrays are copied directly but anything else would be copied through virtual methods.

namespace vtkm
{
namespace cont
{

namespace detail
{

template <typename ValueType, typename InStorage, typename OutStorage>
struct ArrayCopyFunctor
{
  using InArrayHandleType = vtkm::cont::ArrayHandle<ValueType, InStorage>;
  InArrayHandleType InputArray;

  using OutArrayHandleType = vtkm::cont::ArrayHandle<ValueType, OutStorage>;
  OutArrayHandleType OutputArray;

  bool OnlyUseCurrentInputDevice;

  VTKM_CONT
  ArrayCopyFunctor(const InArrayHandleType& input,
                   OutArrayHandleType& output,
                   bool onlyUseCurrentInputDevice)
    : InputArray(input)
    , OutputArray(output)
    , OnlyUseCurrentInputDevice(onlyUseCurrentInputDevice)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    if (this->OnlyUseCurrentInputDevice &&
        (vtkm::cont::DeviceAdapterTraits<Device>::GetId() != this->InputArray.GetDeviceAdapterId()))
    {
      // We were asked to only copy on the device that already has the data from the input array.
      // This is not that device, so return without copying.
      return false;
    }

    vtkm::cont::DeviceAdapterAlgorithm<Device>::Copy(this->InputArray, this->OutputArray);

    return true;
  }
};

} // namespace detail

/// \brief Does a deep copy from one array to another array.
///
/// Given a source \c ArrayHandle and a destination \c ArrayHandle, this function allocates the
/// destination \c ArrayHandle to the correct size and deeply copies all the values from the source
/// to the destination.
///
/// This method optionally takes a \c RuntimeDeviceTracker to control which devices to try.
///
template <typename ValueType, typename InStorage, typename OutStorage>
VTKM_CONT void ArrayCopy(
  const vtkm::cont::ArrayHandle<ValueType, InStorage>& source,
  vtkm::cont::ArrayHandle<ValueType, OutStorage>& destination,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())
{
  bool isCopied = false;

  detail::ArrayCopyFunctor<ValueType, InStorage, OutStorage> functor(source, destination, true);

  // First pass, only use source's already loaded device.
  isCopied = vtkm::cont::TryExecute(functor, tracker);
  if (isCopied)
  {
    return;
  }

  // Second pass, use any available device.
  functor.OnlyUseCurrentInputDevice = false;
  isCopied = vtkm::cont::TryExecute(functor, tracker);
  if (isCopied)
  {
    return;
  }

  // If we are here, then we just failed to copy.
  throw vtkm::cont::ErrorExecution("Failed to run ArrayCopy on any device.");
}

template <typename InValueType, typename InStorage, typename OutValueType, typename OutStorage>
VTKM_CONT void ArrayCopy(
  const vtkm::cont::ArrayHandle<InValueType, InStorage>& source,
  vtkm::cont::ArrayHandle<OutValueType, OutStorage>& destination,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())
{
  ArrayCopy(vtkm::cont::make_ArrayHandleCast<OutValueType>(source), destination, tracker);
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayCopy_h
