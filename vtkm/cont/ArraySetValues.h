//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_ArraySetValues_h
#define vtk_m_cont_ArraySetValues_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <initializer_list>
#include <vector>

namespace vtkm
{
namespace cont
{

namespace internal
{

VTKM_CONT_EXPORT void ArraySetValuesImpl(const vtkm::cont::UnknownArrayHandle& ids,
                                         const vtkm::cont::UnknownArrayHandle& values,
                                         const vtkm::cont::UnknownArrayHandle& data,
                                         std::false_type extractComponentInefficient);

template <typename IdsArrayHandle, typename ValuesArrayHandle, typename DataArrayHandle>
void ArraySetValuesImpl(const IdsArrayHandle& ids,
                        const ValuesArrayHandle& values,
                        const DataArrayHandle& data,
                        std::true_type vtkmNotUsed(extractComponentInefficient))
{
  // Fallback implementation using control portals when device operations would be inefficient
  vtkm::Id numValues = ids.GetNumberOfValues();
  VTKM_ASSERT(values.GetNumberOfValues() == numValues);

  auto idsPortal = ids.ReadPortal();
  auto valuesPortal = values.ReadPortal();
  auto dataPortal = data.WritePortal();

  for (vtkm::Id index = 0; index < numValues; ++index)
  {
    dataPortal.Set(idsPortal.Get(index), valuesPortal.Get(index));
  }
}

} // namespace internal

/// \brief Set a small set of values in an ArrayHandle with minimal device transfers.
///
/// The values in @a values are copied into @a data at the indices specified in @a ids.
/// This is useful for updating a subset of an array on a device without transferring
/// the entire array.
///
/// These functions should not be called repeatedly in a loop to set all values in
/// an array handle. The much more efficient way to do this is to use the proper
/// control-side portals (ArrayHandle::WritePortal()) or to do so in a worklet.
///
/// This method will attempt to copy the data using the device that the input
/// data is already valid on. If the input data is only valid in the control
/// environment or the device copy fails, a control-side copy is performed.
///
/// Since a serial control-side copy may be used, this method is only intended
/// for copying small subsets of the input data. Larger subsets that would
/// benefit from parallelization should prefer using ArrayCopy with an
/// ArrayHandlePermutation.
///
/// This utility provides several convenient overloads:
///
/// A single id and value may be passed into ArraySetValue, or multiple ids and values
/// may be specified to ArraySetValues as ArrayHandles, std::vectors, c-arrays
/// (pointer and size), or as brace-enclosed initializer lists.
///
/// Examples:
///
/// ```cpp
/// vtkm::cont::ArrayHandle<T> data = ...;
///
/// // Set a single value in an array handle:
/// vtkm::cont::ArraySetValue(0, T{42}, data);
///
/// // Set the first and third values in an array handle:
/// vtkm::cont::ArraySetValues({0, 2}, {T{10}, T{30}}, data);
///
/// // Set values using std::vector
/// std::vector<vtkm::Id> ids{0, 1, 2, 3};
/// std::vector<T> values{T{10}, T{20}, T{30}, T{40}};
/// vtkm::cont::ArraySetValues(ids, values, data);
///
/// // Set values using array handles directly
/// vtkm::cont::ArrayHandle<vtkm::Id> idsHandle;
/// vtkm::cont::ArrayHandle<T> valuesHandle;
/// // ... populate handles ...
/// vtkm::cont::ArraySetValues(idsHandle, valuesHandle, data);
///
/// // Set values using raw pointers
/// vtkm::Id rawIds[] = {0, 1, 2};
/// T rawValues[] = {T{10}, T{20}, T{30}};
/// vtkm::cont::ArraySetValues(rawIds, 3, rawValues, 3, data);
/// ```
///
///@{
///
template <typename SIds, typename T, typename SValues, typename SData>
VTKM_CONT void ArraySetValues(const vtkm::cont::ArrayHandle<vtkm::Id, SIds>& ids,
                              const vtkm::cont::ArrayHandle<T, SValues>& values,
                              const vtkm::cont::ArrayHandle<T, SData>& data)
{
  using DataArrayHandle = vtkm::cont::ArrayHandle<T, SData>;
  using InefficientExtract =
    vtkm::cont::internal::ArrayExtractComponentIsInefficient<DataArrayHandle>;
  internal::ArraySetValuesImpl(ids, values, data, InefficientExtract{});
}

/// Specialization for ArrayHandleCasts
template <typename SIds, typename TIn, typename SValues, typename TOut, typename SData>
VTKM_CONT void ArraySetValues(
  const vtkm::cont::ArrayHandle<vtkm::Id, SIds>& ids,
  const vtkm::cont::ArrayHandle<TIn, SValues>& values,
  const vtkm::cont::ArrayHandle<TOut, vtkm::cont::StorageTagCast<TIn, SData>>& data)
{
  vtkm::cont::ArrayHandleBasic<TIn> tempValues;
  tempValues.Allocate(values.GetNumberOfValues());
  auto inp = values.ReadPortal();
  auto outp = tempValues.WritePortal();
  for (vtkm::Id i = 0; i < values.GetNumberOfValues(); ++i)
  {
    outp.Set(i, static_cast<TIn>(inp.Get(i)));
  }

  vtkm::cont::ArrayHandleCast<TOut, vtkm::cont::ArrayHandle<TIn, SData>> castArray = data;
  ArraySetValues(ids, tempValues, castArray.GetSourceArray());
}

template <typename SIds, typename T, typename SData, typename Alloc>
VTKM_CONT void ArraySetValues(const vtkm::cont::ArrayHandle<vtkm::Id, SIds>& ids,
                              const std::vector<T, Alloc>& values,
                              const vtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto valuesAH = vtkm::cont::make_ArrayHandle(values, vtkm::CopyFlag::Off);
  ArraySetValues(ids, valuesAH, data);
}

template <typename T, typename SIds, typename SValues, typename SData>
VTKM_CONT void ArraySetValues(const std::vector<vtkm::Id, SIds>& ids,
                              const vtkm::cont::ArrayHandle<T, SValues>& values,
                              const vtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(ids, vtkm::CopyFlag::Off);
  ArraySetValues(idsAH, values, data);
}

template <typename T, typename AllocId, typename AllocVal, typename SData>
VTKM_CONT void ArraySetValues(const std::vector<vtkm::Id, AllocId>& ids,
                              const std::vector<T, AllocVal>& values,
                              const vtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(ids, vtkm::CopyFlag::Off);
  const auto valuesAH = vtkm::cont::make_ArrayHandle(values, vtkm::CopyFlag::Off);
  ArraySetValues(idsAH, valuesAH, data);
}

template <typename T, typename SData, typename Alloc>
VTKM_CONT void ArraySetValues(const std::initializer_list<vtkm::Id>& ids,
                              const std::vector<T, Alloc>& values,
                              const vtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(
    ids.begin(), static_cast<vtkm::Id>(ids.size()), vtkm::CopyFlag::Off);
  const auto valuesAH = vtkm::cont::make_ArrayHandle(values, vtkm::CopyFlag::Off);
  ArraySetValues(idsAH, valuesAH, data);
}

template <typename T, typename SData>
VTKM_CONT void ArraySetValues(const std::initializer_list<vtkm::Id>& ids,
                              const std::initializer_list<T>& values,
                              const vtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(
    ids.begin(), static_cast<vtkm::Id>(ids.size()), vtkm::CopyFlag::Off);
  const auto valuesAH = vtkm::cont::make_ArrayHandle(
    values.begin(), static_cast<vtkm::Id>(values.size()), vtkm::CopyFlag::Off);
  ArraySetValues(idsAH, valuesAH, data);
}

template <typename T, typename SValues, typename SData>
VTKM_CONT void ArraySetValues(const std::initializer_list<vtkm::Id>& ids,
                              const vtkm::cont::ArrayHandle<T, SValues>& values,
                              const vtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(
    ids.begin(), static_cast<vtkm::Id>(ids.size()), vtkm::CopyFlag::Off);
  ArraySetValues(idsAH, values, data);
}

template <typename T, typename SData>
VTKM_CONT void ArraySetValues(const vtkm::Id* ids,
                              const vtkm::Id numIds,
                              const std::vector<T>& values,
                              const vtkm::cont::ArrayHandle<T, SData>& data)
{
  VTKM_ASSERT(numIds == static_cast<vtkm::Id>(values.size()));
  const auto idsAH = vtkm::cont::make_ArrayHandle(ids, numIds, vtkm::CopyFlag::Off);
  const auto valuesAH =
    vtkm::cont::make_ArrayHandle(values.data(), values.size(), vtkm::CopyFlag::Off);
  ArraySetValues(idsAH, valuesAH, data);
}

template <typename T, typename SData>
VTKM_CONT void ArraySetValues(const vtkm::Id* ids,
                              const vtkm::Id numIds,
                              const T* values,
                              const vtkm::Id numValues,
                              const vtkm::cont::ArrayHandle<T, SData>& data)
{
  VTKM_ASSERT(numIds == numValues);
  const auto idsAH = vtkm::cont::make_ArrayHandle(ids, numIds, vtkm::CopyFlag::Off);
  const auto valuesAH = vtkm::cont::make_ArrayHandle(values, numValues, vtkm::CopyFlag::Off);
  ArraySetValues(idsAH, valuesAH, data);
}

template <typename T, typename SData>
VTKM_CONT void ArraySetValues(const vtkm::Id* ids,
                              const vtkm::Id numIds,
                              const vtkm::cont::ArrayHandle<T, SData>& values,
                              const vtkm::cont::ArrayHandle<T, SData>& data)
{
  VTKM_ASSERT(numIds == values.GetNumberOfValues());
  const auto idsAH = vtkm::cont::make_ArrayHandle(ids, numIds, vtkm::CopyFlag::Off);
  ArraySetValues(idsAH, values, data);
}

/// \brief Set a single value in an ArrayHandle at the specified index.
///
/// This is a convenience function that sets a single value at the given index.
/// It is equivalent to calling ArraySetValues with single-element arrays.
///
template <typename T, typename SData>
VTKM_CONT void ArraySetValue(vtkm::Id id,
                             const T& value,
                             const vtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto idAH = vtkm::cont::make_ArrayHandle(&id, 1, vtkm::CopyFlag::Off);
  const auto valueAH = vtkm::cont::make_ArrayHandle(&value, 1, vtkm::CopyFlag::Off);
  ArraySetValues(idAH, valueAH, data);
}

///@}

} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_ArraySetValues_h
