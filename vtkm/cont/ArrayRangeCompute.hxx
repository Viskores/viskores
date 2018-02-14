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

#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/TryExecute.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

struct ArrayRangeComputeFunctor
{
  template <typename Device, typename ArrayHandleType>
  VTKM_CONT bool operator()(Device,
                            const ArrayHandleType& handle,
                            vtkm::cont::ArrayHandle<vtkm::Range>& range) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using ValueType = typename ArrayHandleType::ValueType;
    using VecTraits = vtkm::VecTraits<ValueType>;
    const vtkm::IdComponent NumberOfComponents = VecTraits::NUM_COMPONENTS;

    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    range.Allocate(NumberOfComponents);

    if (handle.GetNumberOfValues() < 1)
    {
      for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
      {
        range.GetPortalControl().Set(i, vtkm::Range());
      }
      return true;
    }

    vtkm::Vec<ValueType, 2> initial(handle.GetPortalConstControl().Get(0));

    vtkm::Vec<ValueType, 2> result =
      Algorithm::Reduce(handle, initial, vtkm::MinAndMax<ValueType>());

    for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      range.GetPortalControl().Set(
        i,
        vtkm::Range(VecTraits::GetComponent(result[0], i), VecTraits::GetComponent(result[1], i)));
    }

    return true;
  }
};

template <typename ArrayHandleType>
inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeComputeImpl(
  const ArrayHandleType& input,
  vtkm::cont::RuntimeDeviceTracker tracker)
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  vtkm::cont::ArrayHandle<vtkm::Range> range;
  detail::ArrayRangeComputeFunctor functor;

  if (!vtkm::cont::TryExecute(functor, tracker, input, range))
  {
    throw vtkm::cont::ErrorExecution("Failed to run ArrayRangeComputation on any device.");
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
