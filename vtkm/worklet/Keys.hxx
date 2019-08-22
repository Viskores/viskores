//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_Keys_hxx
#define vtk_m_worklet_Keys_hxx

#include <vtkm/worklet/Keys.h>

namespace vtkm
{
namespace worklet
{
/// Build the internal arrays without modifying the input. This is more
/// efficient for stable sorted arrays, but requires an extra copy of the
/// keys for unstable sorting.
template <typename T>
template <typename KeyArrayType>
VTKM_CONT void Keys<T>::BuildArrays(const KeyArrayType& keys,
                                    KeysSortType sort,
                                    vtkm::cont::DeviceAdapterId device)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "Keys::BuildArrays");

  switch (sort)
  {
    case KeysSortType::Unstable:
    {
      KeyArrayHandleType mutableKeys;
      vtkm::cont::Algorithm::Copy(device, keys, mutableKeys);

      this->BuildArraysInternal(mutableKeys, device);
    }
    break;
    case KeysSortType::Stable:
      this->BuildArraysInternalStable(keys, device);
      break;
  }
}

/// Build the internal arrays and also sort the input keys. This is more
/// efficient for unstable sorting, but requires an extra copy for stable
/// sorting.
template <typename T>
template <typename KeyArrayType>
VTKM_CONT void Keys<T>::BuildArraysInPlace(KeyArrayType& keys,
                                           KeysSortType sort,
                                           vtkm::cont::DeviceAdapterId device)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "Keys::BuildArraysInPlace");

  switch (sort)
  {
    case KeysSortType::Unstable:
      this->BuildArraysInternal(keys, device);
      break;
    case KeysSortType::Stable:
    {
      this->BuildArraysInternalStable(keys, device);
      KeyArrayHandleType tmp;
      // Copy into a temporary array so that the permutation array copy
      // won't alias input/output memory:
      vtkm::cont::Algorithm::Copy(device, keys, tmp);
      vtkm::cont::Algorithm::Copy(
        device, vtkm::cont::make_ArrayHandlePermutation(this->SortedValuesMap, tmp), keys);
    }
    break;
  }
}

template <typename T>
template <typename KeyArrayType>
VTKM_CONT void Keys<T>::BuildArraysInternal(KeyArrayType& keys, vtkm::cont::DeviceAdapterId device)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "Keys::BuildArraysInternal");

  const vtkm::Id numKeys = keys.GetNumberOfValues();

  vtkm::cont::Algorithm::Copy(device, vtkm::cont::ArrayHandleIndex(numKeys), this->SortedValuesMap);

  // TODO: Do we need the ability to specify a comparison functor for sort?
  vtkm::cont::Algorithm::SortByKey(device, keys, this->SortedValuesMap);

  // Find the unique keys and the number of values per key.
  vtkm::cont::Algorithm::ReduceByKey(device,
                                     keys,
                                     vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(1, numKeys),
                                     this->UniqueKeys,
                                     this->Counts,
                                     vtkm::Sum());

  // Get the offsets from the counts with a scan.
  vtkm::Id offsetsTotal = vtkm::cont::Algorithm::ScanExclusive(
    device, vtkm::cont::make_ArrayHandleCast(this->Counts, vtkm::Id()), this->Offsets);
  VTKM_ASSERT(offsetsTotal == numKeys); // Sanity check
  (void)offsetsTotal;                   // Shut up, compiler
}

template <typename T>
template <typename KeyArrayType>
VTKM_CONT void Keys<T>::BuildArraysInternalStable(const KeyArrayType& keys,
                                                  vtkm::cont::DeviceAdapterId device)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "Keys::BuildArraysInternalStable");

  const vtkm::Id numKeys = keys.GetNumberOfValues();

  // Produce a stable sorted map of the keys:
  this->SortedValuesMap = StableSortIndices::Sort(device, keys);
  auto sortedKeys = vtkm::cont::make_ArrayHandlePermutation(this->SortedValuesMap, keys);

  // Find the unique keys and the number of values per key.
  vtkm::cont::Algorithm::ReduceByKey(device,
                                     sortedKeys,
                                     vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(1, numKeys),
                                     this->UniqueKeys,
                                     this->Counts,
                                     vtkm::Sum());

  // Get the offsets from the counts with a scan.
  vtkm::Id offsetsTotal = vtkm::cont::Algorithm::ScanExclusive(
    vtkm::cont::make_ArrayHandleCast(this->Counts, vtkm::Id()), this->Offsets);
  VTKM_ASSERT(offsetsTotal == numKeys); // Sanity check
  (void)offsetsTotal;                   // Shut up, compiler
}
}
}
#endif
