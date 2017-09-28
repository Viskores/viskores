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

template <typename ArrayHandleType>
struct ArrayRangeComputeFunctor
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  ArrayHandleType InputArray;
  vtkm::cont::ArrayHandle<vtkm::Range> RangeArray;

  VTKM_CONT
  ArrayRangeComputeFunctor(const ArrayHandleType& input)
    : InputArray(input)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using ValueType = typename ArrayHandleType::ValueType;
    using VecTraits = vtkm::VecTraits<ValueType>;
    const vtkm::IdComponent NumberOfComponents = VecTraits::NUM_COMPONENTS;

    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    this->RangeArray.Allocate(NumberOfComponents);

    if (this->InputArray.GetNumberOfValues() < 1)
    {
      for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
      {
        this->RangeArray.GetPortalControl().Set(i, vtkm::Range());
      }
      return true;
    }

    vtkm::Vec<ValueType, 2> initial(this->InputArray.GetPortalConstControl().Get(0));

    vtkm::Vec<ValueType, 2> result =
      Algorithm::Reduce(this->InputArray, initial, vtkm::MinAndMax<ValueType>());

    for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      this->RangeArray.GetPortalControl().Set(
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

  detail::ArrayRangeComputeFunctor<ArrayHandleType> functor(input);

  if (!vtkm::cont::TryExecute(functor, tracker))
  {
    throw vtkm::cont::ErrorExecution("Failed to run ArrayRangeComputation on any device.");
  }

  return functor.RangeArray;
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
