//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayGetValues_h
#define vtk_m_cont_ArrayGetValues_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <initializer_list>
#include <vector>

namespace vtkm
{
namespace cont
{

// Work around circular dependancy with UnknownArrayHandle.
class UnknownArrayHandle;

namespace internal
{

VTKM_CONT_EXPORT void ArrayGetValuesImpl(const vtkm::cont::UnknownArrayHandle& ids,
                                         const vtkm::cont::UnknownArrayHandle& data,
                                         const vtkm::cont::UnknownArrayHandle& output,
                                         std::false_type extractComponentInefficient);

template <typename IdsArrayHandle, typename DataArrayHandle, typename OutputArrayHandle>
void ArrayGetValuesImpl(const IdsArrayHandle& ids,
                        const DataArrayHandle& data,
                        const OutputArrayHandle& output,
                        std::true_type vtkmNotUsed(extractComponentInefficient))
{
  // Fallback implementation. Using UnknownArrayHandle to extract the data would be more
  // inefficient than simply getting the ReadPortal (which could potentially copy everything
  // form device to host), so we do that here. The only other alternative would be to write
  // a custom worklet, but that would require a device compiler, and we are avoiding that for
  // this header.
  vtkm::Id outputSize = ids.GetNumberOfValues();
  output.Allocate(outputSize);
  auto idsPortal = ids.ReadPortal();
  auto dataPortal = data.ReadPortal();
  auto outputPortal = output.WritePortal();
  for (vtkm::Id index = 0; index < outputSize; ++index)
  {
    outputPortal.Set(index, dataPortal.Get(idsPortal.Get(index)));
  }
}

} // namespace internal

/// \brief Obtain a small set of values from an ArrayHandle with minimal device
/// transfers.
///
/// The values in @a data at the indices in @a ids are copied into a new array
/// and returned. This is useful for retrieving a subset of an array from a
/// device without transferring the entire array to the host.
///
/// These functions should not be called repeatedly in a loop to fetch all
/// values from an array handle. The much more efficient way to do this is to
/// use the proper control-side portals (ArrayHandle::WritePortal() and
/// ArrayHandle::ReadPortal()).
///
/// This method will attempt to copy the data using the device that the input
/// data is already valid on. If the input data is only valid in the control
/// environment or the device copy fails, a control-side copy is performed.
///
/// Since a serial control-side copy may be used, this method is only intended
/// for copying small subsets of the input data. Larger subsets that would
/// benefit from parallelization should prefer using the ArrayCopy method with
/// an ArrayHandlePermutation.
///
/// This utility provides several convenient overloads:
///
/// A single id may be passed into ArrayGetValue, or multiple ids may be
/// specified to ArrayGetValues as an ArrayHandle<vtkm::Id>, a
/// std::vector<vtkm::Id>, a c-array (pointer and size), or as a brace-enclosed
/// initializer list.
///
/// The single result from ArrayGetValue may be returned or written to an output
/// argument. Multiple results from ArrayGetValues may be returned as an
/// std::vector<T>, or written to an output argument as an ArrayHandle<T> or a
/// std::vector<T>.
///
/// Examples:
///
/// ```
/// vtkm::cont::ArrayHandle<T> data = ...;
///
/// // Fetch the first value in an array handle:
/// T firstVal = vtkm::cont::ArrayGetValue(0, data);
///
/// // Fetch the first and third values in an array handle:
/// std::vector<T> firstAndThird = vtkm::cont::ArrayGetValues({0, 2}, data);
///
/// // Fetch the first and last values in an array handle:
/// std::vector<T> firstAndLast =
///     vtkm::cont::ArrayGetValues({0, data.GetNumberOfValues() - 1}, data);
///
/// // Fetch the first 4 values into an array handle:
/// const std::vector<vtkm::Id> ids{0, 1, 2, 3};
/// vtkm::cont::ArrayHandle<T> firstFour;
/// vtkm::cont::ArrayGetValues(ids, data, firstFour);
/// ```
///
///
///@{
///
template <typename SIds, typename T, typename SData, typename SOut>
VTKM_CONT void ArrayGetValues(const vtkm::cont::ArrayHandle<vtkm::Id, SIds>& ids,
                              const vtkm::cont::ArrayHandle<T, SData>& data,
                              vtkm::cont::ArrayHandle<T, SOut>& output)
{
  VTKM_STATIC_ASSERT_MSG(
    vtkm::HasVecTraits<T>::value,
    "ArrayGetValues can only be used with arrays containing value types with VecTraits defined.");
  using DataArrayHandle = vtkm::cont::ArrayHandle<T, SData>;
  using InefficientExtract =
    vtkm::cont::internal::ArrayExtractComponentIsInefficient<DataArrayHandle>;
  internal::ArrayGetValuesImpl(ids, data, output, InefficientExtract{});
}

/// We need a specialization for `ArrayHandleCasts` to avoid runtime type missmatch errors inside
/// `ArrayGetValuesImpl`.
template <typename SIds, typename TIn, typename SData, typename TOut, typename SOut>
VTKM_CONT void ArrayGetValues(
  const vtkm::cont::ArrayHandle<vtkm::Id, SIds>& ids,
  const vtkm::cont::ArrayHandle<TOut, vtkm::cont::StorageTagCast<TIn, SData>>& data,
  vtkm::cont::ArrayHandle<TOut, SOut>& output)
{
  // In this specialization, we extract the values from the cast array's source array and
  // then cast and copy to output.
  vtkm::cont::ArrayHandleBasic<TIn> tempOutput;
  vtkm::cont::ArrayHandleCast<TOut, vtkm::cont::ArrayHandle<TIn, SData>> castArray = data;
  ArrayGetValues(ids, castArray.GetSourceArray(), tempOutput);

  vtkm::Id numExtracted = tempOutput.GetNumberOfValues();
  output.Allocate(numExtracted);
  auto inp = tempOutput.ReadPortal();
  auto outp = output.WritePortal();
  for (vtkm::Id i = 0; i < numExtracted; ++i)
  {
    outp.Set(i, static_cast<TOut>(inp.Get(i)));
  }
}

template <typename SIds, typename T, typename SData, typename Alloc>
VTKM_CONT void ArrayGetValues(const vtkm::cont::ArrayHandle<vtkm::Id, SIds>& ids,
                              const vtkm::cont::ArrayHandle<T, SData>& data,
                              std::vector<T, Alloc>& output)
{
  const std::size_t numVals = static_cast<std::size_t>(ids.GetNumberOfValues());

  // Allocate the vector and toss its data pointer into the array handle.
  output.resize(numVals);
  auto result = vtkm::cont::make_ArrayHandle(output, vtkm::CopyFlag::Off);
  vtkm::cont::ArrayGetValues(ids, data, result);
  // Make sure to pull the data back to control before we dealloc the handle
  // that wraps the vec memory:
  result.SyncControlArray();
}

template <typename SIds, typename T, typename SData>
VTKM_CONT std::vector<T> ArrayGetValues(const vtkm::cont::ArrayHandle<vtkm::Id, SIds>& ids,
                                        const vtkm::cont::ArrayHandle<T, SData>& data)
{
  std::vector<T> result;
  vtkm::cont::ArrayGetValues(ids, data, result);
  return result;
}

template <typename T, typename Alloc, typename SData, typename SOut>
VTKM_CONT void ArrayGetValues(const std::vector<vtkm::Id, Alloc>& ids,
                              const vtkm::cont::ArrayHandle<T, SData>& data,
                              vtkm::cont::ArrayHandle<T, SOut>& output)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(ids, vtkm::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}

template <typename T, typename AllocId, typename SData, typename AllocOut>
VTKM_CONT void ArrayGetValues(const std::vector<vtkm::Id, AllocId>& ids,
                              const vtkm::cont::ArrayHandle<T, SData>& data,
                              std::vector<T, AllocOut>& output)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(ids, vtkm::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}

template <typename T, typename Alloc, typename SData>
VTKM_CONT std::vector<T> ArrayGetValues(const std::vector<vtkm::Id, Alloc>& ids,
                                        const vtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(ids, vtkm::CopyFlag::Off);
  return ArrayGetValues(idsAH, data);
}

template <typename T, typename SData, typename SOut>
VTKM_CONT void ArrayGetValues(const std::initializer_list<vtkm::Id>& ids,
                              const vtkm::cont::ArrayHandle<T, SData>& data,
                              vtkm::cont::ArrayHandle<T, SOut>& output)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(
    ids.begin(), static_cast<vtkm::Id>(ids.size()), vtkm::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}

template <typename T, typename SData, typename Alloc>
VTKM_CONT void ArrayGetValues(const std::initializer_list<vtkm::Id>& ids,
                              const vtkm::cont::ArrayHandle<T, SData>& data,
                              std::vector<T, Alloc>& output)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(
    ids.begin(), static_cast<vtkm::Id>(ids.size()), vtkm::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}
template <typename T, typename SData>
VTKM_CONT std::vector<T> ArrayGetValues(const std::initializer_list<vtkm::Id>& ids,
                                        const vtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(
    ids.begin(), static_cast<vtkm::Id>(ids.size()), vtkm::CopyFlag::Off);
  return ArrayGetValues(idsAH, data);
}

template <typename T, typename SData, typename SOut>
VTKM_CONT void ArrayGetValues(const vtkm::Id* ids,
                              const vtkm::Id numIds,
                              const vtkm::cont::ArrayHandle<T, SData>& data,
                              vtkm::cont::ArrayHandle<T, SOut>& output)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(ids, numIds, vtkm::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}

template <typename T, typename SData, typename Alloc>
VTKM_CONT void ArrayGetValues(const vtkm::Id* ids,
                              const vtkm::Id numIds,
                              const vtkm::cont::ArrayHandle<T, SData>& data,
                              std::vector<T, Alloc>& output)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(ids, numIds, vtkm::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}
template <typename T, typename SData>
VTKM_CONT std::vector<T> ArrayGetValues(const vtkm::Id* ids,
                                        const vtkm::Id numIds,
                                        const vtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = vtkm::cont::make_ArrayHandle(ids, numIds, vtkm::CopyFlag::Off);
  return ArrayGetValues(idsAH, data);
}

template <typename T, typename S>
VTKM_CONT T ArrayGetValue(vtkm::Id id, const vtkm::cont::ArrayHandle<T, S>& data)
{
  const auto idAH = vtkm::cont::make_ArrayHandle(&id, 1, vtkm::CopyFlag::Off);
  auto result = vtkm::cont::ArrayGetValues(idAH, data);
  return result[0];
}

template <typename T, typename S>
VTKM_CONT void ArrayGetValue(vtkm::Id id, const vtkm::cont::ArrayHandle<T, S>& data, T& val)
{
  val = ArrayGetValue(id, data);
}
/// @}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayGetValues_h
