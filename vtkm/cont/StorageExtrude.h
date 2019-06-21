//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_internal_StorageExtrude_h
#define vtkm_internal_StorageExtrude_h

#include <vtkm/cont/ArrayPortalExtrude.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

namespace vtkm
{
namespace cont
{
namespace internal
{
struct VTKM_ALWAYS_EXPORT StorageTagExtrude
{
};

template <typename T>
class Storage<T, internal::StorageTagExtrude>
{
  using BaseT = typename BaseComponent<T>::Type;
  using HandleType = vtkm::cont::ArrayHandle<BaseT>;
  using TPortalType = typename HandleType::PortalConstControl;

public:
  using ValueType = T;

  // This is meant to be invalid. Because point arrays are read only, you
  // should only be able to use the const version.
  struct PortalType
  {
    using ValueType = void*;
    using IteratorType = void*;
  };

  using PortalConstType = exec::ArrayPortalExtrude<TPortalType>;

  Storage()
    : Array()
    , Length(-1)
    , NumberOfPlanes(0)
  {
  }

  // Create with externally managed memory
  Storage(const BaseT* array, vtkm::Id arrayLength, vtkm::Int32 numberOfPlanes, bool cylindrical)
    : Array(vtkm::cont::make_ArrayHandle(array, arrayLength))
    , Length(static_cast<vtkm::Int32>(arrayLength))
    , NumberOfPlanes(numberOfPlanes)
    , UseCylindrical(cylindrical)
  {
    VTKM_ASSERT(this->Length >= 0);
  }

  Storage(const HandleType& array, vtkm::Int32 numberOfPlanes, bool cylindrical)
    : Array(array)
    , Length(static_cast<vtkm::Int32>(array.GetNumberOfValues()))
    , NumberOfPlanes(numberOfPlanes)
    , UseCylindrical(cylindrical)
  {
    VTKM_ASSERT(this->Length >= 0);
  }

  PortalType GetPortal() { return PortalType{}; }

  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Length >= 0);
    return PortalConstType(this->Array.GetPortalConstControl(),
                           this->Length,
                           this->NumberOfPlanes,
                           this->UseCylindrical);
  }

  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Length >= 0);
    return (this->Length / 2) * static_cast<vtkm::Id>(this->NumberOfPlanes);
  }

  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("StorageTagExtrude is read only. It cannot be allocated.");
  }

  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("StoraageTagExtrue is read only. It cannot shrink.");
  }

  void ReleaseResources()
  {
    // This request is ignored since we don't own the memory that was past
    // to us
  }

  vtkm::cont::ArrayHandle<BaseT> Array;
  vtkm::Int32 Length;
  vtkm::Int32 NumberOfPlanes;
  bool UseCylindrical;
};

template <typename T, typename Device>
class VTKM_ALWAYS_EXPORT ArrayTransfer<T, internal::StorageTagExtrude, Device>
{
  using BaseT = typename BaseComponent<T>::Type;
  using TPortalType = decltype(vtkm::cont::ArrayHandle<BaseT>{}.PrepareForInput(Device{}));

public:
  using ValueType = T;
  using StorageType = vtkm::cont::internal::Storage<T, internal::StorageTagExtrude>;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  //meant to be an invalid writeable execution portal
  using PortalExecution = typename StorageType::PortalType;

  using PortalConstExecution = vtkm::exec::ArrayPortalExtrude<TPortalType>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : ControlData(storage)
  {
  }
  vtkm::Id GetNumberOfValues() const { return this->ControlData->GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->ControlData->Array.PrepareForInput(Device()),
                                this->ControlData->Length,
                                this->ControlData->NumberOfPlanes,
                                this->ControlData->UseCylindrical);
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool& vtkmNotUsed(updateData))
  {
    throw vtkm::cont::ErrorBadType("StorageExtrude read only. "
                                   "Cannot be used for in-place operations.");
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("StorageExtrude read only. Cannot be used as output.");
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandleExrPointCoordinates read only. "
      "There should be no occurance of the ArrayHandle trying to pull "
      "data from the execution environment.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("StorageExtrude read only. Cannot shrink.");
  }

  VTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since we don't own the memory that was past
    // to us
  }

private:
  const StorageType* const ControlData;
};
}
}
}

#endif
