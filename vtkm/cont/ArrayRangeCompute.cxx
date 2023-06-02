//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayRangeCompute.h>
#include <vtkm/cont/ArrayRangeComputeTemplate.h>

#include <vtkm/TypeList.h>

#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/ArrayHandleStride.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleXGCCoordinates.h>

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

using CartesianProductStorage = vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                                                       vtkm::cont::StorageTagBasic,
                                                                       vtkm::cont::StorageTagBasic>;

using StorageTagsList = vtkm::List<vtkm::cont::StorageTagBasic,
                                   vtkm::cont::StorageTagSOA,
                                   vtkm::cont::StorageTagXGCCoordinates,
                                   vtkm::cont::StorageTagUniformPoints,
                                   CartesianProductStorage,
                                   vtkm::cont::StorageTagConstant,
                                   vtkm::cont::StorageTagCounting,
                                   vtkm::cont::StorageTagIndex>;

template <typename StorageTag>
struct StorageTagToValueTypesMap;

#define MAP_STORAGE_TAG_VALUE_TYPES(StorageTag, ValueTypesList) \
  template <>                                                   \
  struct StorageTagToValueTypesMap<StorageTag>                  \
  {                                                             \
    using TypeList = ValueTypesList;                            \
  }

MAP_STORAGE_TAG_VALUE_TYPES(vtkm::cont::StorageTagBasic, AllTypes);
MAP_STORAGE_TAG_VALUE_TYPES(vtkm::cont::StorageTagSOA, AllVec);
MAP_STORAGE_TAG_VALUE_TYPES(vtkm::cont::StorageTagXGCCoordinates, vtkm::TypeListFieldVec3);
MAP_STORAGE_TAG_VALUE_TYPES(vtkm::cont::StorageTagUniformPoints, vtkm::List<vtkm::Vec3f>);
MAP_STORAGE_TAG_VALUE_TYPES(CartesianProductStorage, vtkm::TypeListFieldVec3);
MAP_STORAGE_TAG_VALUE_TYPES(vtkm::cont::StorageTagConstant, AllTypes);
MAP_STORAGE_TAG_VALUE_TYPES(vtkm::cont::StorageTagCounting, AllTypes);
MAP_STORAGE_TAG_VALUE_TYPES(vtkm::cont::StorageTagIndex, vtkm::List<vtkm::Id>);

#undef MAP_STORAGE_TAG_VALUE_TYPES

} // anonymous namespace

namespace vtkm
{
namespace cont
{

namespace internal
{

void ThrowArrayRangeComputeFailed()
{
  throw vtkm::cont::ErrorExecution("Failed to run ArrayRangeComputation on any device.");
}

} // namespace internal

vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(const vtkm::cont::UnknownArrayHandle& array,
                                                       bool computeFiniteRange,
                                                       vtkm::cont::DeviceAdapterId device)
{
  return ArrayRangeCompute(
    array, vtkm::cont::ArrayHandle<vtkm::UInt8>{}, computeFiniteRange, device);
}

vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::UnknownArrayHandle& array,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  bool computeFiniteRange,
  vtkm::cont::DeviceAdapterId device)
{
  // First, try (potentially fast-paths) for common(ish) array types.
  try
  {
    vtkm::cont::ArrayHandle<vtkm::Range> ranges;

    auto computeForArrayHandle = [&](const auto& input) {
      ranges = vtkm::cont::ArrayRangeComputeTemplate(input, maskArray, computeFiniteRange, device);
    };

    bool success = false;
    auto computeForStorage = [&](auto storageTag) {
      using STag = decltype(storageTag);
      using VTypes = typename StorageTagToValueTypesMap<STag>::TypeList;
      if (array.IsStorageType<STag>())
      {
        array.CastAndCallForTypes<VTypes, vtkm::List<STag>>(computeForArrayHandle);
        success = true;
      }
    };

    vtkm::ListForEach(computeForStorage, StorageTagsList{});

    if (success)
    {
      return ranges;
    }
  }
  catch (vtkm::cont::ErrorBadType&)
  {
    // If a cast/call failed, try falling back to a more general implementation.
  }

  // fallback
  bool success = false;
  vtkm::cont::ArrayHandle<vtkm::Range> ranges;
  auto computeForExtractComponent = [&](auto valueTypeObj) {
    using VT = decltype(valueTypeObj);
    if (!success && array.IsBaseComponentType<VT>())
    {
      vtkm::IdComponent numComponents = array.GetNumberOfComponentsFlat();
      ranges.Allocate(numComponents);
      auto rangePortal = ranges.WritePortal();
      for (vtkm::IdComponent i = 0; i < numComponents; ++i)
      {
        auto componentArray = array.ExtractComponent<VT>(i);
        auto componentRange = vtkm::cont::ArrayRangeComputeTemplate(
          componentArray, maskArray, computeFiniteRange, device);
        rangePortal.Set(i, componentRange.ReadPortal().Get(0));
      }
      success = true;
    }
  };

  vtkm::ListForEach(computeForExtractComponent, AllScalars{});
  if (!success)
  {
    internal::ThrowArrayRangeComputeFailed();
  }

  return ranges;
}

vtkm::Range ArrayRangeComputeMagnitude(const vtkm::cont::UnknownArrayHandle& array,
                                       bool computeFiniteRange,
                                       vtkm::cont::DeviceAdapterId device)
{
  return ArrayRangeComputeMagnitude(
    array, vtkm::cont::ArrayHandle<vtkm::UInt8>{}, computeFiniteRange, device);
}

vtkm::Range ArrayRangeComputeMagnitude(const vtkm::cont::UnknownArrayHandle& array,
                                       const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
                                       bool computeFiniteRange,
                                       vtkm::cont::DeviceAdapterId device)
{
  // First, try (potentially fast-paths) for common(ish) array types.
  try
  {
    vtkm::Range range;

    auto computeForArrayHandle = [&](const auto& input) {
      range = vtkm::cont::ArrayRangeComputeMagnitudeTemplate(
        input, maskArray, computeFiniteRange, device);
    };

    bool success = false;
    auto computeForStorage = [&](auto storageTag) {
      using STag = decltype(storageTag);
      using VTypes = typename StorageTagToValueTypesMap<STag>::TypeList;
      if (array.IsStorageType<STag>())
      {
        array.CastAndCallForTypes<VTypes, vtkm::List<STag>>(computeForArrayHandle);
        success = true;
      }
    };

    vtkm::ListForEach(computeForStorage, StorageTagsList{});

    if (success)
    {
      return range;
    }
  }
  catch (vtkm::cont::ErrorBadType&)
  {
    // If a cast/call failed, try falling back to a more general implementation.
  }

  // fallback
  bool success = false;
  vtkm::Range range;
  auto computeForExtractArrayFromComponents = [&](auto valueTypeObj) {
    using VT = decltype(valueTypeObj);
    if (!success && array.IsBaseComponentType<VT>())
    {
      auto extractedArray = array.ExtractArrayFromComponents<VT>();
      range = vtkm::cont::ArrayRangeComputeMagnitudeTemplate(
        extractedArray, maskArray, computeFiniteRange, device);
      success = true;
    }
  };

  vtkm::ListForEach(computeForExtractArrayFromComponents, AllScalars{});
  if (!success)
  {
    internal::ThrowArrayRangeComputeFailed();
  }

  return range;
}

}
} // namespace vtkm::cont
