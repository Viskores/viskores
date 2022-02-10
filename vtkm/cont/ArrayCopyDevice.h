//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayCopyDevice_h
#define vtk_m_cont_ArrayCopyDevice_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/cont/internal/ArrayHandleDeprecated.h>

#include <vtkm/cont/vtkm_cont_export.h>

// TODO: When virtual arrays are available, compile the implementation in a .cxx/.cu file. Common
// arrays are copied directly but anything else would be copied through virtual methods.

namespace vtkm
{
namespace cont
{

namespace detail
{

// Element-wise copy.
template <typename InArrayType, typename OutArrayType>
void ArrayCopyWithAlgorithm(const InArrayType& source, OutArrayType& destination)
{
  // Current implementation of Algorithm::Copy will first try to copy on devices where the
  // data is already available.
  vtkm::cont::Algorithm::Copy(source, destination);
}

// TODO: Remove last argument once ArryHandleNewStyle becomes ArrayHandle
template <typename InArrayType, typename OutArrayType>
void ArrayCopyOldImpl(const InArrayType& in, OutArrayType& out, std::false_type /* Copy storage */)
{
  ArrayCopyWithAlgorithm(in, out);
}

// Copy storage for implicit arrays, must be of same type:
// TODO: This will go away once ArrayHandleNewStyle becomes ArrayHandle.
template <typename ArrayType>
void ArrayCopyOldImpl(const ArrayType& in, ArrayType& out, std::true_type /* Copy storage */)
{
  // This is only called if in/out are the same type and the handle is not
  // writable. This allows read-only implicit array handles to be copied.
  auto newStorage = in.GetStorage();
  out = ArrayType(newStorage);
}

// TODO: This will go away once ArrayHandleNewStyle becomes ArrayHandle.
template <typename InArrayType, typename OutArrayType>
VTKM_CONT void ArrayCopyImpl(const InArrayType& source,
                             OutArrayType& destination,
                             std::false_type /* New style */)
{
  using SameTypes = std::is_same<InArrayType, OutArrayType>;
  using IsWritable = vtkm::cont::internal::IsWritableArrayHandle<OutArrayType>;
  using JustCopyStorage = std::integral_constant<bool, SameTypes::value && !IsWritable::value>;
  ArrayCopyOldImpl(source, destination, JustCopyStorage{});
}

// TODO: ArrayHandleNewStyle will eventually become ArrayHandle, in which case this
// will become ArrayCopyWithAlgorithm
template <typename T1, typename S1, typename T2, typename S2>
VTKM_CONT void ArrayCopyImpl(const vtkm::cont::ArrayHandle<T1, S1>& source,
                             vtkm::cont::ArrayHandle<T2, S2>& destination,
                             std::true_type /* New style */)
{
  VTKM_STATIC_ASSERT((!std::is_same<T1, T2>::value || !std::is_same<S1, S2>::value));
  ArrayCopyWithAlgorithm(source, destination);
}

// TODO: ArrayHandleNewStyle will eventually become ArrayHandle, in which case this
// will become the only version with the same array types.
template <typename T, typename S>
VTKM_CONT void ArrayCopyImpl(const vtkm::cont::ArrayHandle<T, S>& source,
                             vtkm::cont::ArrayHandle<T, S>& destination,
                             std::true_type /* New style */)
{
  destination.DeepCopyFrom(source);
}

} // namespace detail

/// \brief Does a deep copy from one array to another array.
///
/// Given a source `ArrayHandle` and a destination `ArrayHandle`, this
/// function allocates the destination `ArrayHandle` to the correct size and
/// deeply copies all the values from the source to the destination.
///
/// This method will attempt to copy the data using the device that the input
/// data is already valid on. If the input data is only valid in the control
/// environment, the runtime device tracker is used to try to find another
/// device.
///
/// This should work on some non-writable array handles as well, as long as
/// both \a source and \a destination are the same type.
///
/// This version of array copy is templated to create custom code for the
/// particular types of `ArrayHandle`s that you are copying. This will
/// ensure that you get the best possible copy, but requires a device
/// compiler and tends to bloat the code.
///
/// @{
///
template <typename InValueType, typename InStorage, typename OutValueType, typename OutStorage>
VTKM_CONT void ArrayCopyDevice(const vtkm::cont::ArrayHandle<InValueType, InStorage>& source,
                               vtkm::cont::ArrayHandle<OutValueType, OutStorage>& destination)
{
  using InArrayType = vtkm::cont::ArrayHandle<InValueType, InStorage>;
  using OutArrayType = vtkm::cont::ArrayHandle<OutValueType, OutStorage>;
  using SameTypes = std::is_same<InArrayType, OutArrayType>;
  using IsWritable = vtkm::cont::internal::IsWritableArrayHandle<OutArrayType>;

  // There are three cases handled here:
  // 1. The arrays are the same type:
  //    -> Deep copy the buffers and the Storage object
  // 2. The arrays are different and the output is writable:
  //    -> Do element-wise copy
  // 3. The arrays are different and the output is not writable:
  //    -> fail (cannot copy)

  // Give a nice error message for case 3:
  VTKM_STATIC_ASSERT_MSG(IsWritable::value || SameTypes::value,
                         "Cannot copy to a read-only array with a different "
                         "type than the source.");

  using IsOldStyle =
    std::is_base_of<vtkm::cont::internal::ArrayHandleDeprecated<InValueType, InStorage>,
                    InArrayType>;

  // Static dispatch cases 1 & 2
  detail::ArrayCopyImpl(source, destination, std::integral_constant<bool, !IsOldStyle::value>{});
}

/// @}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayCopyDevice_h
