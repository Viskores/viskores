//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayCopy_h
#define vtk_m_cont_ArrayCopy_h

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
// TODO: Remove last argument once ArryHandleNewStyle becomes ArrayHandle
template <typename InArrayType, typename OutArrayType>
void ArrayCopyWithAlgorithm(const InArrayType& source, OutArrayType& destination)
{
  // Find the device that already has a copy of the data:
  vtkm::cont::DeviceAdapterId devId = source.GetDeviceAdapterId();

  // If the data is not on any device, let the runtime tracker pick an available
  // parallel copy algorithm.
  if (devId.GetValue() == VTKM_DEVICE_ADAPTER_UNDEFINED)
  {
    devId = vtkm::cont::make_DeviceAdapterId(VTKM_DEVICE_ADAPTER_ANY);
  }

  bool success = vtkm::cont::Algorithm::Copy(devId, source, destination);

  if (!success && devId.GetValue() != VTKM_DEVICE_ADAPTER_ANY)
  { // Retry on any device if the first attempt failed.
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "Failed to run ArrayCopy on device '" << devId.GetName()
                                                     << "'. Retrying on any device.");
    success = vtkm::cont::Algorithm::Copy(vtkm::cont::DeviceAdapterTagAny{}, source, destination);
  }

  if (!success)
  {
    throw vtkm::cont::ErrorExecution("Failed to run ArrayCopy on any device.");
  }
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
/// Given a source \c ArrayHandle and a destination \c ArrayHandle, this
/// function allocates the destination \c ArrayHandle to the correct size and
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
template <typename InValueType, typename InStorage, typename OutValueType, typename OutStorage>
VTKM_CONT void ArrayCopy(const vtkm::cont::ArrayHandle<InValueType, InStorage>& source,
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


VTKM_CONT_EXPORT void ArrayCopy(const vtkm::cont::UnknownArrayHandle& source,
                                vtkm::cont::UnknownArrayHandle& destination);

template <typename T, typename S>
VTKM_CONT void ArrayCopy(const vtkm::cont::UnknownArrayHandle& source,
                         vtkm::cont::ArrayHandle<T, S>& destination)
{
  using DestType = vtkm::cont::ArrayHandle<T, S>;
  if (source.IsType<DestType>())
  {
    ArrayCopy(source.AsArrayHandle<DestType>(), destination);
  }
  else
  {
    vtkm::cont::UnknownArrayHandle destWrapper(destination);
    ArrayCopy(source, destWrapper);
    // Destination array should not change, but just in case.
    destWrapper.AsArrayHandle(destination);
  }
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayCopy_h
