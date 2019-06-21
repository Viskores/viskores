//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_internal_ArrayPortalExtrudePlane_h
#define vtk_m_internal_ArrayPortalExtrudePlane_h

#include <vtkm/internal/IndicesExtrude.h>

#include <vtkm/cont/ArrayHandle.h>


namespace vtkm
{
namespace exec
{

template <typename PortalType>
struct VTKM_ALWAYS_EXPORT ArrayPortalExtrudePlane
{
  using ValueType = typename PortalType::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalExtrudePlane()
    : Portal()
    , NumberOfPlanes(0){};

  ArrayPortalExtrudePlane(const PortalType& p, vtkm::Int32 numOfPlanes)
    : Portal(p)
    , NumberOfPlanes(numOfPlanes)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->Portal.GetNumberOfValues() * static_cast<vtkm::Id>(NumberOfPlanes);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return this->Portal.Get(index % this->NumberOfPlanes); }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id2 index) const { return this->Portal.Get(index[0]); }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Vec<ValueType, 6> GetWedge(const IndicesExtrude& index) const;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void Set(vtkm::Id vtkmNotUsed(index), const ValueType& vtkmNotUsed(value)) const {}

  PortalType Portal;
  vtkm::Int32 NumberOfPlanes;
};
}
} // vtkm::exec

namespace vtkm
{
namespace cont
{
namespace internal
{

struct VTKM_ALWAYS_EXPORT StorageTagExtrudePlane
{
};

template <typename T>
class VTKM_ALWAYS_EXPORT Storage<T, internal::StorageTagExtrudePlane>
{
  using HandleType = vtkm::cont::ArrayHandle<T>;

public:
  using ValueType = T;

  // This is meant to be invalid. Because point arrays are read only, you
  // should only be able to use the const version.
  struct PortalType
  {
    using ValueType = void*;
    using IteratorType = void*;
  };

  using PortalConstType =
    vtkm::exec::ArrayPortalExtrudePlane<typename HandleType::PortalConstControl>;

  Storage()
    : Array()
    , NumberOfPlanes(0)
  {
  }

  Storage(const HandleType& array, vtkm::Int32 numberOfPlanes)
    : Array(array)
    , NumberOfPlanes(numberOfPlanes)
  {
  }

  PortalType GetPortal() { return PortalType{}; }

  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->Array.GetPortalConstControl(), this->NumberOfPlanes);
  }

  vtkm::Id GetNumberOfValues() const
  {
    return this->Array.GetNumberOfValues() * static_cast<vtkm::Id>(this->NumberOfPlanes);
  }

  vtkm::Int32 GetNumberOfValuesPerPlane() const
  {
    return static_cast<vtkm::Int32>(this->Array->GetNumberOfValues());
  }

  vtkm::Int32 GetNumberOfPlanes() const { return this->NumberOfPlanes; }

  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayPortalExtrudePlane is read only. It cannot be allocated.");
  }

  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayPortalExtrudePlane is read only. It cannot shrink.");
  }

  void ReleaseResources()
  {
    // This request is ignored since we don't own the memory that was past
    // to us
  }

private:
  vtkm::cont::ArrayHandle<T> Array;
  vtkm::Int32 NumberOfPlanes;
};

template <typename T, typename Device>
class VTKM_ALWAYS_EXPORT ArrayTransfer<T, internal::StorageTagExtrudePlane, Device>
{
public:
  using ValueType = T;
  using StorageType = vtkm::cont::internal::Storage<T, internal::StorageTagExtrudePlane>;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  //meant to be an invalid writeable execution portal
  using PortalExecution = typename StorageType::PortalType;
  using PortalConstExecution = vtkm::exec::ArrayPortalExtrudePlane<decltype(
    vtkm::cont::ArrayHandle<T>{}.PrepareForInput(Device{}))>;

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
                                this->ControlData->NumberOfPlanes);
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool& vtkmNotUsed(updateData))
  {
    throw vtkm::cont::ErrorBadType("ArrayPortalExtrudePlane read only. "
                                   "Cannot be used for in-place operations.");
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayPortalExtrudePlane read only. Cannot be used as output.");
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayPortalExtrudePlane read only. "
      "There should be no occurance of the ArrayHandle trying to pull "
      "data from the execution environment.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayPortalExtrudePlane read only. Cannot shrink.");
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
} // vtkm::cont::internal

#endif
