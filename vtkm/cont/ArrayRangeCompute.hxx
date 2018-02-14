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
#ifndef vtk_m_cont_ArrayRangeCompute_hxx
#define vtk_m_cont_ArrayRangeCompute_hxx

#include <vtkm/cont/ArrayRangeCompute.h>

#include <vtkm/BinaryOperators.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/Algorithm.h>

#include <limits>

namespace vtkm
{
namespace cont
{

namespace detail
{

struct ArrayRangeComputeFunctor
{
  template <typename Device, typename T, typename S>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, S>& handle,
                            const vtkm::Vec<T, 2>& initialValue,
                            vtkm::Vec<T, 2>& result) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;
    result = Algorithm::Reduce(handle, initialValue, vtkm::MinAndMax<T>());
    return true;
  }
};

template <typename T, typename S>
inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeComputeImpl(
  const vtkm::cont::ArrayHandle<T, S>& input,
  vtkm::cont::RuntimeDeviceTracker& tracker)
{
  using VecTraits = vtkm::VecTraits<T>;
  using CT = typename VecTraits::ComponentType;
  //We want to minimize the amount of code that we do in try execute as
  //it is repeated for each
  vtkm::cont::ArrayHandle<vtkm::Range> range;
  range.Allocate(VecTraits::NUM_COMPONENTS);

  if (input.GetNumberOfValues() < 1)
  {
    auto portal = range.GetPortalControl();
    for (vtkm::IdComponent i = 0; i < VecTraits::NUM_COMPONENTS; ++i)
    {
      portal.Set(i, vtkm::Range());
    }
  }
  else
  {
    //We used the limits, so that we don't need to sync the array handle
    //
    vtkm::Vec<T, 2> result;
    vtkm::Vec<T, 2> initial;
    initial[0] = T(std::numeric_limits<CT>::max());
    initial[1] = T(std::numeric_limits<CT>::lowest());

    const bool rangeComputed =
      vtkm::cont::TryExecute(detail::ArrayRangeComputeFunctor{}, tracker, input, initial, result);
    if (!rangeComputed)
    {
      ThrowArrayRangeComputeFailed();
    }
    else
    {
      auto portal = range.GetPortalControl();
      for (vtkm::IdComponent i = 0; i < VecTraits::NUM_COMPONENTS; ++i)
      {
        portal.Set(i,
                   vtkm::Range(VecTraits::GetComponent(result[0], i),
                               VecTraits::GetComponent(result[1], i)));
      }
    }
  }
  return range;
}

} // namespace detail

template <typename ArrayHandleType>
inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const ArrayHandleType& input,
  vtkm::cont::RuntimeDeviceTracker tracker)
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  return detail::ArrayRangeComputeImpl(input, tracker);
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayRangeCompute_hxx
