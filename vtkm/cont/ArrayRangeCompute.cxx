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

#include <vtkm/TypeList.h>

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

VTKM_ARRAY_RANGE_COMPUTE_IMPL_ALL_SCALAR_T(vtkm::cont::StorageTagStride);

VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Float32, 3, vtkm::cont::StorageTagXGCCoordinates);
VTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(vtkm::Float64, 3, vtkm::cont::StorageTagXGCCoordinates);

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

vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagIndex>& input,
  vtkm::cont::DeviceAdapterId)
{
  vtkm::cont::ArrayHandle<vtkm::Range> result;
  result.Allocate(1);
  result.WritePortal().Set(0, vtkm::Range(0, input.GetNumberOfValues() - 1));
  return result;
}

namespace
{

using AllScalars = vtkm::TypeListBaseC;

template <typename vtkm::IdComponent N>
struct VecTransform
{
  template <typename T>
  using type = vtkm::Vec<T, N>;
};

template <vtkm::IdComponent N>
using AllVecOfSize = vtkm::ListTransform<AllScalars, VecTransform<N>::template type>;

using AllVec = vtkm::ListAppend<AllVecOfSize<2>, AllVecOfSize<3>, AllVecOfSize<4>>;

using AllTypes = vtkm::ListAppend<AllScalars, AllVec>;

struct ComputeRangeFunctor
{
  // Used with UnknownArrayHandle::CastAndCallForTypes
  template <typename T, typename S>
  void operator()(const vtkm::cont::ArrayHandle<T, S>& array,
                  vtkm::cont::DeviceAdapterId device,
                  vtkm::cont::ArrayHandle<vtkm::Range>& ranges) const
  {
    ranges = vtkm::cont::ArrayRangeCompute(array, device);
  }

  // Used with vtkm::ListForEach to get components
  template <typename T>
  void operator()(T,
                  const vtkm::cont::UnknownArrayHandle& array,
                  vtkm::cont::DeviceAdapterId device,
                  vtkm::cont::ArrayHandle<vtkm::Range>& ranges,
                  bool& success) const
  {
    if (!success && array.IsBaseComponentType<T>())
    {
      vtkm::IdComponent numComponents = array.GetNumberOfComponentsFlat();
      ranges.Allocate(numComponents);
      auto rangePortal = ranges.WritePortal();
      for (vtkm::IdComponent componentI = 0; componentI < numComponents; ++componentI)
      {
        vtkm::cont::ArrayHandleStride<T> componentArray = array.ExtractComponent<T>(componentI);
        vtkm::cont::ArrayHandle<vtkm::Range> componentRange =
          vtkm::cont::ArrayRangeCompute(componentArray, device);
        rangePortal.Set(componentI, componentRange.ReadPortal().Get(0));
      }
      success = true;
    }
  }
};

template <typename TList, typename Storage>
vtkm::cont::ArrayHandle<vtkm::Range> ComputeForStorage(const vtkm::cont::UnknownArrayHandle& array,
                                                       vtkm::cont::DeviceAdapterId device)
{
  vtkm::cont::ArrayHandle<vtkm::Range> ranges;
  array.CastAndCallForTypes<TList, vtkm::List<Storage>>(ComputeRangeFunctor{}, device, ranges);
  return ranges;
}

} // anonymous namespace

vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(const vtkm::cont::UnknownArrayHandle& array,
                                                       vtkm::cont::DeviceAdapterId device)
{
  // First, try fast-paths of precompiled array types common(ish) in fields.
  try
  {
    if (array.IsStorageType<vtkm::cont::StorageTagBasic>())
    {
      return ComputeForStorage<AllTypes, vtkm::cont::StorageTagBasic>(array, device);
    }
    if (array.IsStorageType<vtkm::cont::StorageTagSOA>())
    {
      return ComputeForStorage<AllVec, vtkm::cont::StorageTagSOA>(array, device);
    }
    if (array.IsStorageType<vtkm::cont::StorageTagXGCCoordinates>())
    {
      return ComputeForStorage<vtkm::TypeListFieldVec3, vtkm::cont::StorageTagXGCCoordinates>(
        array, device);
    }
    if (array.IsStorageType<vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag>())
    {
      vtkm::cont::ArrayHandleUniformPointCoordinates uniformPoints;
      array.AsArrayHandle(uniformPoints);
      return vtkm::cont::ArrayRangeCompute(uniformPoints, device);
    }
    using CartesianProductStorage =
      vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                             vtkm::cont::StorageTagBasic,
                                             vtkm::cont::StorageTagBasic>;
    if (array.IsStorageType<CartesianProductStorage>())
    {
      return ComputeForStorage<vtkm::TypeListFieldVec3, CartesianProductStorage>(array, device);
    }
    if (array.IsStorageType<vtkm::cont::StorageTagConstant>())
    {
      return ComputeForStorage<AllTypes, vtkm::cont::StorageTagConstant>(array, device);
    }
    if (array.IsStorageType<vtkm::cont::StorageTagCounting>())
    {
      return ComputeForStorage<AllTypes, vtkm::cont::StorageTagCounting>(array, device);
    }
    if (array.IsStorageType<vtkm::cont::StorageTagIndex>())
    {
      return ArrayRangeCompute(array.AsArrayHandle<vtkm::cont::ArrayHandleIndex>(), device);
    }
  }
  catch (vtkm::cont::ErrorBadType&)
  {
    // If a cast/call failed, try falling back to a more general implementation.
  }

  vtkm::cont::ArrayHandle<vtkm::Range> ranges;
  bool success = false;
  vtkm::ListForEach(ComputeRangeFunctor{}, AllScalars{}, array, device, ranges, success);
  return ranges;
}

}
} // namespace vtkm::cont
