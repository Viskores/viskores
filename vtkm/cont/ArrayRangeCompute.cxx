//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayRangeComputeTemplate.h>

namespace vtkm
{
namespace cont
{

void ThrowArrayRangeComputeFailed()
{
  throw vtkm::cont::ErrorExecution("Failed to run ArrayRangeComputation on any device.");
}

#define VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(T, Storage)                                       \
  VTKM_CONT                                                                               \
  vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(                                 \
    const vtkm::cont::ArrayHandle<T, Storage>& input, vtkm::cont::DeviceAdapterId device) \
  {                                                                                       \
    return detail::ArrayRangeComputeImpl(input, device);                                  \
  }                                                                                       \
  struct SwallowSemicolon
#define VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(T, N, Storage)            \
  VTKM_CONT                                                         \
  vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, Storage>& input, \
    vtkm::cont::DeviceAdapterId device)                             \
  {                                                                 \
    return detail::ArrayRangeComputeImpl(input, device);            \
  }                                                                 \
  struct SwallowSemicolon

#define VTKM_ARRAY_RANGE_COMPUTE_IMPL_ALL_SCALAR_T(Storage)              \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::Int8, Storage);                  \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::UInt8, Storage);                 \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::Int16, Storage);                 \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::UInt16, Storage);                \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::Int32, Storage);                 \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::UInt32, Storage);                \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::Int64, Storage);                 \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::UInt64, Storage);                \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::Float32, Storage);               \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(vtkm::Float64, Storage);               \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(char, Storage);                        \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(signed VTKM_UNUSED_INT_TYPE, Storage); \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_T(unsigned VTKM_UNUSED_INT_TYPE, Storage)

#define VTKM_ARRAY_RANGE_COMPUTE_IMPL_ALL_VEC(N, Storage)                     \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Int8, N, Storage);                  \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::UInt8, N, Storage);                 \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Int16, N, Storage);                 \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::UInt16, N, Storage);                \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Int32, N, Storage);                 \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::UInt32, N, Storage);                \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Int64, N, Storage);                 \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::UInt64, N, Storage);                \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Float32, N, Storage);               \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Float64, N, Storage);               \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(char, N, Storage);                        \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(signed VTKM_UNUSED_INT_TYPE, N, Storage); \
  VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(unsigned VTKM_UNUSED_INT_TYPE, N, Storage)

VTKM_ARRAY_RANGE_COMPUTE_IMPL_ALL_SCALAR_T(vtkm::cont::StorageTagBasic);

VTKM_ARRAY_RANGE_COMPUTE_IMPL_ALL_VEC(2, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_ALL_VEC(3, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_ALL_VEC(4, vtkm::cont::StorageTagBasic);

VTKM_ARRAY_RANGE_COMPUTE_IMPL_ALL_VEC(2, vtkm::cont::StorageTagSOA);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_ALL_VEC(3, vtkm::cont::StorageTagSOA);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_ALL_VEC(4, vtkm::cont::StorageTagSOA);

#undef VTKM_ARRAY_RANGE_COMPUTE_IMPL_T
#undef VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC
#undef VTKM_ARRAY_RANGE_COMPUTE_IMPL_ALL_SCALAR_T
#undef VTKM_ARRAY_RANGE_COMPUTE_IMPL_ALL_VEC

// Special implementation for regular point coordinates, which are easy
// to determine.
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<vtkm::Vec3f,
                                vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag>& array,
  vtkm::cont::DeviceAdapterId)
{
  vtkm::internal::ArrayPortalUniformPointCoordinates portal = array.ReadPortal();

  // In this portal we know that the min value is the first entry and the
  // max value is the last entry.
  vtkm::Vec3f minimum = portal.Get(0);
  vtkm::Vec3f maximum = portal.Get(portal.GetNumberOfValues() - 1);

  vtkm::cont::ArrayHandle<vtkm::Range> rangeArray;
  rangeArray.Allocate(3);
  vtkm::cont::ArrayHandle<vtkm::Range>::WritePortalType outPortal = rangeArray.WritePortal();
  outPortal.Set(0, vtkm::Range(minimum[0], maximum[0]));
  outPortal.Set(1, vtkm::Range(minimum[1], maximum[1]));
  outPortal.Set(2, vtkm::Range(minimum[2], maximum[2]));

  return rangeArray;
}

VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagIndex>& input,
  vtkm::cont::DeviceAdapterId)
{
  vtkm::cont::ArrayHandle<vtkm::Range> result;
  result.Allocate(1);
  result.WritePortal().Set(0, vtkm::Range(0, input.GetNumberOfValues() - 1));
  return result;
}
}
} // namespace vtkm::cont
