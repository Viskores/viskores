//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleDiscard_h
#define vtk_m_cont_ArrayHandleDiscard_h

#include <vtkm/TypeTraits.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/internal/Unreachable.h>

#include <type_traits>

namespace vtkm
{
namespace exec
{
namespace internal
{

/// \brief An output-only array portal with no storage. All written values are
/// discarded.
template <typename ValueType_>
class ArrayPortalDiscard
{
public:
  using ValueType = ValueType_;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalDiscard()
    : NumberOfValues(0)
  {
  } // needs to be host and device so that cuda can create lvalue of these

  VTKM_CONT
  explicit ArrayPortalDiscard(vtkm::Id numValues)
    : NumberOfValues(numValues)
  {
  }

  /// Copy constructor for any other ArrayPortalDiscard with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template <class OtherV>
  VTKM_CONT ArrayPortalDiscard(const ArrayPortalDiscard<OtherV>& src)
    : NumberOfValues(src.NumberOfValues)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  ValueType Get(vtkm::Id) const
  {
    VTKM_UNREACHABLE("Cannot read from ArrayHandleDiscard.");
    return vtkm::TypeTraits<ValueType>::ZeroInitialization();
  }

  VTKM_EXEC
  void Set(vtkm::Id index, const ValueType&) const
  {
    VTKM_ASSERT(index < this->GetNumberOfValues());
    (void)index;
    // no-op
  }

private:
  vtkm::Id NumberOfValues;
};

} // end namespace internal
} // end namespace exec

namespace cont
{

namespace internal
{

struct VTKM_ALWAYS_EXPORT StorageTagDiscard
{
};

template <typename ValueType_>
class Storage<ValueType_, StorageTagDiscard>
{
public:
  using ValueType = ValueType_;
  using PortalType = vtkm::exec::internal::ArrayPortalDiscard<ValueType>;
  using PortalConstType = vtkm::exec::internal::ArrayPortalDiscard<ValueType>;

  VTKM_CONT
  Storage() {}

  VTKM_CONT
  PortalType GetPortal() { return PortalType(this->NumberOfValues); }

  VTKM_CONT
  PortalConstType GetPortalConst() { return PortalConstType(this->NumberOfValues); }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_CONT
  void Allocate(vtkm::Id numValues) { this->NumberOfValues = numValues; }

  VTKM_CONT
  void Shrink(vtkm::Id numValues) { this->NumberOfValues = numValues; }

  VTKM_CONT
  void ReleaseResources() { this->NumberOfValues = 0; }

private:
  vtkm::Id NumberOfValues;
};

template <typename ValueType_, typename DeviceAdapter_>
class ArrayTransfer<ValueType_, StorageTagDiscard, DeviceAdapter_>
{
  using StorageTag = StorageTagDiscard;
  using StorageType = Storage<ValueType_, StorageTag>;

public:
  using ValueType = ValueType_;
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;
  using PortalExecution = vtkm::exec::internal::ArrayPortalDiscard<ValueType>;
  using PortalConstExecution = vtkm::exec::internal::ArrayPortalDiscard<ValueType>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Internal(storage)
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Internal != nullptr);
    return this->Internal->GetNumberOfValues();
  }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    throw vtkm::cont::ErrorBadValue("Input access not supported: "
                                    "Cannot read from an ArrayHandleDiscard.");
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    throw vtkm::cont::ErrorBadValue("InPlace access not supported: "
                                    "Cannot read from an ArrayHandleDiscard.");
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numValues)
  {
    VTKM_ASSERT(this->Internal != nullptr);
    this->Internal->Allocate(numValues);
    return PortalConstExecution(this->Internal->GetNumberOfValues());
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* storage) const
  {
    VTKM_ASSERT(storage == this->Internal);
    (void)storage;
    // no-op
  }

  VTKM_CONT
  void Shrink(vtkm::Id numValues)
  {
    VTKM_ASSERT(this->Internal != nullptr);
    this->Internal->Shrink(numValues);
  }

  VTKM_CONT
  void ReleaseResources()
  {
    VTKM_ASSERT(this->Internal != nullptr);
    this->Internal->ReleaseResources();
  }

private:
  StorageType* Internal;
};

template <typename ValueType_>
struct ArrayHandleDiscardTraits
{
  using ValueType = ValueType_;
  using StorageTag = StorageTagDiscard;
  using Superclass = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
};

} // end namespace internal

/// ArrayHandleDiscard is a write-only array that discards all data written to
/// it. This can be used to save memory when a filter provides optional outputs
/// that are not needed.
template <typename ValueType_>
class ArrayHandleDiscard : public internal::ArrayHandleDiscardTraits<ValueType_>::Superclass
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleDiscard,
                             (ArrayHandleDiscard<ValueType_>),
                             (typename internal::ArrayHandleDiscardTraits<ValueType_>::Superclass));
};

/// Helper to determine if an ArrayHandle type is an ArrayHandleDiscard.
template <typename T>
struct IsArrayHandleDiscard : std::false_type
{
};

template <typename T>
struct IsArrayHandleDiscard<ArrayHandle<T, internal::StorageTagDiscard>> : std::true_type
{
};

} // end namespace cont
} // end namespace vtkm

#endif // vtk_m_cont_ArrayHandleDiscard_h
