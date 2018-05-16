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

#include <vtkm/cont/ArrayRangeCompute.hxx>

namespace vtkm
{
namespace cont
{

void ThrowArrayRangeComputeFailed()
{
  throw vtkm::cont::ErrorExecution("Failed to run ArrayRangeComputation on any device.");
}

#define VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(T, Storage)                                                \
  VTKM_CONT                                                                                        \
  vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(                                          \
    const vtkm::cont::ArrayHandle<T, Storage>& input, vtkm::cont::RuntimeDeviceTracker tracker)    \
  {                                                                                                \
    return detail::ArrayRangeComputeImpl(input, tracker);                                          \
  }                                                                                                \
  struct SwallowSemicolon
#define VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(T, N, Storage)                                           \
  VTKM_CONT                                                                                        \
  vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(                                          \
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, Storage>& input,                                \
    vtkm::cont::RuntimeDeviceTracker tracker)                                                      \
  {                                                                                                \
    return detail::ArrayRangeComputeImpl(input, tracker);                                          \
  }                                                                                                \
  struct SwallowSemicolon

VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(char, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::Int8, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::UInt8, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::Int16, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::UInt16, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::Int32, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::UInt32, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::Int64, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::UInt64, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::Float32, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::Float64, vtkm::cont::StorageTagBasic);

VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Int32, 2, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Int64, 2, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Float32, 2, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Float64, 2, vtkm::cont::StorageTagBasic);

VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Int32, 3, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Int64, 3, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Float32, 3, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Float64, 3, vtkm::cont::StorageTagBasic);

VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(char, 4, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Int8, 4, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::UInt8, 4, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Float32, 4, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Float64, 4, vtkm::cont::StorageTagBasic);

VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::FloatDefault,
                                  3,
                                  vtkm::cont::ArrayHandleVirtualCoordinates::StorageTag);

#undef VTKM_ARRAY_RANGE_COMPUTE_IMPL_T
#undef VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC

// Special implementation for regular point coordinates, which are easy
// to determine.
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>,
                                vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag>& array,
  vtkm::cont::RuntimeDeviceTracker)
{
  vtkm::internal::ArrayPortalUniformPointCoordinates portal = array.GetPortalConstControl();

  // In this portal we know that the min value is the first entry and the
  // max value is the last entry.
  vtkm::Vec<vtkm::FloatDefault, 3> minimum = portal.Get(0);
  vtkm::Vec<vtkm::FloatDefault, 3> maximum = portal.Get(portal.GetNumberOfValues() - 1);

  vtkm::cont::ArrayHandle<vtkm::Range> rangeArray;
  rangeArray.Allocate(3);
  vtkm::cont::ArrayHandle<vtkm::Range>::PortalControl outPortal = rangeArray.GetPortalControl();
  outPortal.Set(0, vtkm::Range(minimum[0], maximum[0]));
  outPortal.Set(1, vtkm::Range(minimum[1], maximum[1]));
  outPortal.Set(2, vtkm::Range(minimum[2], maximum[2]));

  return rangeArray;
}

// Special implementation for composite vectors.
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>,
                                typename vtkm::cont::ArrayHandleCompositeVector<
                                  vtkm::cont::ArrayHandle<vtkm::Float32>,
                                  vtkm::cont::ArrayHandle<vtkm::Float32>,
                                  vtkm::cont::ArrayHandle<vtkm::Float32>>::StorageTag>& input,
  vtkm::cont::RuntimeDeviceTracker tracker)
{
  return detail::ArrayRangeComputeImpl(input, tracker);
}

VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>,
                                typename vtkm::cont::ArrayHandleCompositeVector<
                                  vtkm::cont::ArrayHandle<vtkm::Float64>,
                                  vtkm::cont::ArrayHandle<vtkm::Float64>,
                                  vtkm::cont::ArrayHandle<vtkm::Float64>>::StorageTag>& input,
  vtkm::cont::RuntimeDeviceTracker tracker)
{
  return detail::ArrayRangeComputeImpl(input, tracker);
}
}
} // namespace vtkm::cont
