//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayRangeCompute_h
#define vtk_m_cont_ArrayRangeCompute_h

#include <vtkm/Range.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/UnknownArrayHandle.h>

namespace vtkm
{
namespace cont
{

/// @{
/// @brief Compute the range of the data in an array handle.
///
/// Given an `ArrayHandle`, this function computes the range (min and max) of
/// the values in the array. For arrays containing Vec values, the range is
/// computed for each component, and in the case of nested Vecs, ranges are computed
/// for each of the leaf components.
///
/// The `array` parameter is the input array as a `vtkm::cont::UnknownArrayHandle`.
///
/// The optional `maskArray` parameter supports selectively choosing which entries to
/// include in the range. It is an array handle of type `vtkm::cont::ArrayHandle<vtkm::UInt8>`.
/// This array should have the same number of elements as the input array
/// with each value representing the masking status of the corresponding
/// value in the input array (masked if 0 else unmasked). Ignored if empty.
///
/// The optional `computeFiniteRange` parameter specifies whether if non-finite
/// values in the array should be ignored to compute the finite range of
/// the array. For Vec types, individual component values are considered independantly.
///
/// The optional `device` parameter can be used to specify a device to run the
/// range computation on. The default value is `vtkm::cont::DeviceAdapterTagAny{}`.
///
/// @return The result is returned in an `ArrayHandle` of `Range` objects. There is
/// one value in the returned array for every component of the input's value
/// type. For nested Vecs the results are stored in depth-first order.
///
/// @note `ArrayRangeCompute` takes an UnknownArrayHandle as the input.
/// The implementation uses precompiled and specicialized code for several of the
/// most commonly used value and storage types, with a fallback for other cases.
/// This is so that ArrayRangeCompute.h can be included in code that does not use a
/// device compiler. This should be sufficient for most cases, but if you need to
/// compute the range for an array type that is not explicitly handled by
/// `ArrayRangeCompute` and the fallback code is not performant, use the
/// templated version `ArrayRangeComputeTemplate`. Specializations can be
/// implemented by specializing the template class `ArrayRangeComputeImpl`.
/// Please refer to ArrayRangeComputeTemplate.h for details
///
/// @sa ArrayRangeComputeTemplate
///

VTKM_CONT_EXPORT vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::UnknownArrayHandle& array,
  bool computeFiniteRange = false,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{});

VTKM_CONT_EXPORT vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::UnknownArrayHandle& array,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  bool computeFiniteRange = false,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{});

inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::UnknownArrayHandle& array,
  vtkm::cont::DeviceAdapterId device)
{
  return ArrayRangeCompute(array, false, device);
}

/// @}

/// @{
/// \brief Compute the range of the magnitude of the Vec data in an array handle.
///
/// Given an `ArrayHandle`, this function computes the range (min and max) of
/// the magnitude of the values in the array.
///
///
/// The `array` parameter is the input array as a `vtkm::cont::UnknownArrayHandle`.
///
/// The optional `maskArray` parameter supports selectively choosing which entries to
/// include in the range. It is an array handle of type `vtkm::cont::ArrayHandle<vtkm::UInt8>`.
/// This array should have the same number of elements as the input array
/// with each value representing the masking status of the corresponding
/// value in the input array (masked if 0 else unmasked). Ignored if empty.
///
/// The optional `computeFiniteRange` parameter specifies whether if non-finite
/// values in the array should be ignored to compute the finite range of
/// the array. A Vec with any non-finite component will be ignored.
///
/// The optional `device` parameter can be used to specify a device to run the
/// range computation on. The default value is `vtkm::cont::DeviceAdapterTagAny{}`.
///
/// \return The result is returned in a single `Range` objects.
///
/// \note `ArrayRangeComputeMagnitude` takes an UnknownArrayHandle as the input.
/// The implementation uses precompiled and specicialized code for several of the
/// most commonly used value and storage types, with a fallback for other cases.
/// This is so that ArrayRangeCompute.h can be included in code that does not use a
/// device compiler. This should be sufficient for most cases, but if you need to
/// compute the range for an array type that is not explicitly handled by
/// `ArrayRangeComputeMagnitude` and the fallback code is not performant, use the
/// templated version `ArrayRangeComputeMagnitudeTemplate`. Specializations can be
/// implemented by specializing the template class `ArrayRangeComputeMagnitudeImpl`.
/// Please refer to ArrayRangeComputeTemplate.h for details
///
/// \sa ArrayRangeComputeMagnitudeTemplate
///
VTKM_CONT_EXPORT vtkm::Range ArrayRangeComputeMagnitude(
  const vtkm::cont::UnknownArrayHandle& array,
  bool computeFiniteRange = false,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{});

VTKM_CONT_EXPORT vtkm::Range ArrayRangeComputeMagnitude(
  const vtkm::cont::UnknownArrayHandle& array,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  bool computeFiniteRange = false,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{});

inline vtkm::Range ArrayRangeComputeMagnitude(const vtkm::cont::UnknownArrayHandle& array,
                                              vtkm::cont::DeviceAdapterId device)
{
  return ArrayRangeComputeMagnitude(array, false, device);
}

/// @}

namespace internal
{

VTKM_CONT_EXPORT void ThrowArrayRangeComputeFailed();

} // namespace internal

VTKM_DEPRECATED(2.1, "Moved to vtkm::cont::internal.")
inline void ThrowArrayRangeComputeFailed()
{
  internal::ThrowArrayRangeComputeFailed();
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayRangeCompute_h
