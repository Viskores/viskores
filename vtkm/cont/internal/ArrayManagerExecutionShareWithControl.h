//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ArrayManagerExecutionShareWithControl_h
#define vtk_m_cont_internal_ArrayManagerExecutionShareWithControl_h

#include <vtkm/Assert.h>
#include <vtkm/Types.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Storage.h>

#include <algorithm>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// \c ArrayManagerExecutionShareWithControl provides an implementation for a
/// \c ArrayManagerExecution class for a device adapter when the execution
/// and control environments share memory. This class basically defers all its
/// calls to a \c Storage class and uses the array allocated there.
///
template <typename T, class StorageTag>
class ArrayManagerExecutionShareWithControl
{
public:
  using ValueType = T;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;
  using PortalType = typename StorageType::PortalType;
  using PortalConstType = typename StorageType::PortalConstType;

  VTKM_CONT
  ArrayManagerExecutionShareWithControl(StorageType* storage)
    : Storage(storage)
  {
  }

  /// Returns the size of the storage.
  ///
  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  /// Returns the constant portal from the storage.
  ///
  VTKM_CONT
  PortalConstType PrepareForInput(bool vtkmNotUsed(uploadData)) const
  {
    return this->Storage->GetPortalConst();
  }

  /// Returns the read-write portal from the storage.
  ///
  VTKM_CONT
  PortalType PrepareForInPlace(bool vtkmNotUsed(uploadData)) { return this->Storage->GetPortal(); }

  /// Allocates data in the storage and return the portal to that.
  ///
  VTKM_CONT
  PortalType PrepareForOutput(vtkm::Id numberOfValues)
  {
    this->Storage->Allocate(numberOfValues);
    return this->Storage->GetPortal();
  }

  /// This method is a no-op (except for a few checks). Any data written to
  /// this class's portals should already be written to the given \c
  /// controlArray (under correct operation).
  ///
  VTKM_CONT
  void RetrieveOutputData(StorageType* storage) const
  {
    (void)storage;
    VTKM_ASSERT(storage == this->Storage);
  }

  /// Shrinks the storage.
  ///
  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { this->Storage->Shrink(numberOfValues); }

  /// A no-op.
  ///
  VTKM_CONT
  void ReleaseResources() {}

private:
  ArrayManagerExecutionShareWithControl(ArrayManagerExecutionShareWithControl<T, StorageTag>&) =
    delete;
  void operator=(ArrayManagerExecutionShareWithControl<T, StorageTag>&) = delete;

  StorageType* Storage;
};

// Specializations for basic storage:
template <typename T>
struct ExecutionPortalFactoryBasicShareWithControl
{
  using ValueType = T;
  using PortalType = ArrayPortalFromIterators<ValueType*>;
  using PortalConstType = ArrayPortalFromIterators<const ValueType*>;

  VTKM_CONT
  static PortalType CreatePortal(ValueType* start, ValueType* end)
  {
    return PortalType(start, end);
  }

  VTKM_CONT
  static PortalConstType CreatePortalConst(const ValueType* start, const ValueType* end)
  {
    return PortalConstType(start, end);
  }
};

struct VTKM_CONT_EXPORT ExecutionArrayInterfaceBasicShareWithControl
  : public ExecutionArrayInterfaceBasicBase
{
  //inherit our parents constructor
  using ExecutionArrayInterfaceBasicBase::ExecutionArrayInterfaceBasicBase;

  VTKM_CONT void Allocate(TypelessExecutionArray& execArray,
                          vtkm::Id numberOfValues,
                          vtkm::UInt64 sizeOfValue) const final;
  VTKM_CONT void Free(TypelessExecutionArray& execArray) const final;

  VTKM_CONT void CopyFromControl(const void* src, void* dst, vtkm::UInt64 bytes) const final;
  VTKM_CONT void CopyToControl(const void* src, void* dst, vtkm::UInt64 bytes) const final;

  VTKM_CONT void UsingForRead(const void* controlPtr,
                              const void* executionPtr,
                              vtkm::UInt64 numBytes) const final;
  VTKM_CONT void UsingForWrite(const void* controlPtr,
                               const void* executionPtr,
                               vtkm::UInt64 numBytes) const final;
  VTKM_CONT void UsingForReadWrite(const void* controlPtr,
                                   const void* executionPtr,
                                   vtkm::UInt64 numBytes) const final;
};
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ArrayManagerExecutionShareWithControl_h
