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

#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
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

struct ComputeRangeFunctor
{
  // Used with UnknownArrayHandle::CastAndCallForTypes
  template <typename T, typename S>
  void operator()(const vtkm::cont::ArrayHandle<T, S>& array,
                  vtkm::cont::DeviceAdapterId device,
                  vtkm::cont::ArrayHandle<vtkm::Range>& ranges) const
  {
    ranges = vtkm::cont::ArrayRangeComputeTemplate(array, device);
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
          vtkm::cont::ArrayRangeComputeTemplate(componentArray, device);
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
      return vtkm::cont::ArrayRangeComputeTemplate(uniformPoints, device);
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
      return ArrayRangeComputeTemplate(array.AsArrayHandle<vtkm::cont::ArrayHandleIndex>(), device);
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
