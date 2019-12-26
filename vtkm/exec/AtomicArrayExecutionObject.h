//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_AtomicArrayExecutionObject_h
#define vtk_m_exec_AtomicArrayExecutionObject_h

#include <vtkm/List.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/internal/AtomicInterfaceExecution.h>

#include <type_traits>

namespace vtkm
{
namespace exec
{

namespace detail
{
// Clang-7 as host compiler under nvcc returns types from std::make_unsigned
// that are not compatible with the AtomicInterface API, so we define our own
// mapping. This must exist for every entry in vtkm::cont::AtomicArrayTypeList.
template <typename>
struct MakeUnsigned;
template <>
struct MakeUnsigned<vtkm::UInt32>
{
  using type = vtkm::UInt32;
};
template <>
struct MakeUnsigned<vtkm::Int32>
{
  using type = vtkm::UInt32;
};
template <>
struct MakeUnsigned<vtkm::UInt64>
{
  using type = vtkm::UInt64;
};
template <>
struct MakeUnsigned<vtkm::Int64>
{
  using type = vtkm::UInt64;
};
}

template <typename T, typename Device>
class AtomicArrayExecutionObject
{
  using AtomicInterface = vtkm::cont::internal::AtomicInterfaceExecution<Device>;

  // Checks if PortalType has a GetIteratorBegin() method that returns a
  // pointer.
  template <typename PortalType,
            typename PointerType = decltype(std::declval<PortalType>().GetIteratorBegin())>
  struct HasPointerAccess : public std::is_pointer<PointerType>
  {
  };

public:
  using ValueType = T;

  AtomicArrayExecutionObject() = default;

  VTKM_CONT AtomicArrayExecutionObject(vtkm::cont::ArrayHandle<T> handle)
    : Data{ handle.PrepareForInPlace(Device{}).GetIteratorBegin() }
    , NumberOfValues{ handle.GetNumberOfValues() }
  {
    using PortalType = decltype(handle.PrepareForInPlace(Device{}));
    VTKM_STATIC_ASSERT_MSG(HasPointerAccess<PortalType>::value,
                           "Source portal must return a pointer from "
                           "GetIteratorBegin().");
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  /// \brief Perform an atomic load of the indexed element with acquire memory
  /// ordering.
  /// \param index The index of the element to load.
  /// \return The value of the atomic array at \a index.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Get(vtkm::Id index) const
  {
    // We only support 32/64 bit signed/unsigned ints, and AtomicInterface
    // currently only provides API for unsigned types.
    // We'll cast the signed types to unsigned to work around this.
    using APIType = typename detail::MakeUnsigned<ValueType>::type;

    return static_cast<T>(
      AtomicInterface::Load(reinterpret_cast<const APIType*>(this->Data + index)));
  }

  /// \brief Peform an atomic addition with sequentially consistent memory
  /// ordering.
  /// \param index The index of the array element that will be added to.
  /// \param value The addend of the atomic add operation.
  /// \return The original value of the element at \a index (before addition).
  /// \warning Overflow behavior from this operation is undefined.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Add(vtkm::Id index, const ValueType& value) const
  {
    // We only support 32/64 bit signed/unsigned ints, and AtomicInterface
    // currently only provides API for unsigned types.
    // We'll cast the signed types to unsigned to work around this.
    // This is safe, since the only difference between signed/unsigned types
    // is how overflow works, and signed overflow is already undefined. We also
    // document that overflow is undefined for this operation.
    using APIType = typename detail::MakeUnsigned<ValueType>::type;

    return static_cast<T>(AtomicInterface::Add(reinterpret_cast<APIType*>(this->Data + index),
                                               static_cast<APIType>(value)));
  }

  /// \brief Peform an atomic store to memory while enforcing, at minimum, "release"
  /// memory ordering.
  /// \param index The index of the array element that will be added to.
  /// \param value The value to write for the atomic store operation.
  /// \warning Using something like:
  /// ```
  /// Set(index, Get(index)+N)
  /// ```
  /// Should not be done as it is not thread safe, instead you should use
  /// the provided Add method instead.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void Set(vtkm::Id index, const ValueType& value) const
  {
    // We only support 32/64 bit signed/unsigned ints, and AtomicInterface
    // currently only provides API for unsigned types.
    // We'll cast the signed types to unsigned to work around this.
    // This is safe, since the only difference between signed/unsigned types
    // is how overflow works, and signed overflow is already undefined. We also
    // document that overflow is undefined for this operation.
    using APIType = typename detail::MakeUnsigned<ValueType>::type;

    AtomicInterface::Store(reinterpret_cast<APIType*>(this->Data + index),
                           static_cast<APIType>(value));
  }

  /// \brief Perform an atomic CAS operation with sequentially consistent
  /// memory ordering.
  /// \param index The index of the array element that will be atomically
  /// modified.
  /// \param newValue The value to replace the indexed element with.
  /// \param oldValue The expected value of the indexed element.
  /// \return If the operation is successful, \a oldValue is returned. Otherwise
  /// the current value of the indexed element is returned, and the element is
  /// not modified.
  ///
  /// This operation is typically used in a loop. For example usage,
  /// an atomic multiplication may be implemented using CAS as follows:
  ///
  /// ```
  /// AtomicArrayExecutionObject<vtkm::Int32, ...> arr = ...;
  ///
  /// // CAS multiplication:
  /// vtkm::Int32 cur = arr->Get(idx); // Load the current value at idx
  /// vtkm::Int32 newVal; // will hold the result of the multiplication
  /// vtkm::Int32 expect; // will hold the expected value before multiplication
  /// do {
  ///   expect = cur; // Used to ensure the value hasn't changed since reading
  ///   newVal = cur * multFactor; // the actual multiplication
  /// }
  /// while ((cur = arr->CompareAndSwap(idx, newVal, expect)) == expect);
  /// ```
  ///
  /// The while condition here updates \a cur with the pre-CAS value of the
  /// operation (the return from CompareAndSwap), and compares this to the
  /// expected value. If the values match, the operation was successful and the
  /// loop exits. If the values do not match, the value at \a idx was changed
  /// by another thread since the initial Get, and the CAS operation failed --
  /// the target element was not modified by the CAS call. If this happens, the
  /// loop body re-executes using the new value of \a cur and tries again until
  /// it succeeds.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType CompareAndSwap(vtkm::Id index,
                           const ValueType& newValue,
                           const ValueType& oldValue) const
  {
    // We only support 32/64 bit signed/unsigned ints, and AtomicInterface
    // currently only provides API for unsigned types.
    // We'll cast the signed types to unsigned to work around this.
    // This is safe, since the only difference between signed/unsigned types
    // is how overflow works, and signed overflow is already undefined.
    using APIType = typename detail::MakeUnsigned<ValueType>::type;

    return static_cast<T>(
      AtomicInterface::CompareAndSwap(reinterpret_cast<APIType*>(this->Data + index),
                                      static_cast<APIType>(newValue),
                                      static_cast<APIType>(oldValue)));
  }

private:
  ValueType* Data{ nullptr };
  vtkm::Id NumberOfValues{ 0 };
};
}
} // namespace vtkm::exec

#endif //vtk_m_exec_AtomicArrayExecutionObject_h
