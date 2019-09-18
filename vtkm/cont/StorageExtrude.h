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

#include <vtkm/internal/IndicesExtrude.h>

#include <vtkm/VecTraits.h>

#include <vtkm/cont/ErrorBadType.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

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
  vtkm::Vec<ValueType, 6> GetWedge(const IndicesExtrude& index) const
  {
    vtkm::Vec<ValueType, 6> result;
    result[0] = this->Portal.Get(index.PointIds[0][0]);
    result[1] = this->Portal.Get(index.PointIds[0][1]);
    result[2] = this->Portal.Get(index.PointIds[0][2]);
    result[3] = this->Portal.Get(index.PointIds[1][0]);
    result[4] = this->Portal.Get(index.PointIds[1][1]);
    result[5] = this->Portal.Get(index.PointIds[1][2]);

    return result;
  }

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

  using PortalConstType =
    vtkm::exec::ArrayPortalExtrudePlane<typename HandleType::PortalConstControl>;

  // Note that this array is read only, so you really should only be getting the const
  // version of the portal. If you actually try to write to this portal, you will
  // get an error.
  using PortalType = PortalConstType;

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

  PortalType GetPortal()
  {
    throw vtkm::cont::ErrorBadType(
      "Extrude ArrayHandles are read only. Cannot get writable portal.");
  }

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

namespace vtkm
{
namespace exec
{

template <typename PortalType>
struct VTKM_ALWAYS_EXPORT ArrayPortalExtrude
{
  using ValueType = vtkm::Vec<typename PortalType::ValueType, 3>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalExtrude()
    : Portal()
    , NumberOfValues(0)
    , NumberOfPlanes(0)
    , UseCylindrical(false){};

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalExtrude(const PortalType& p,
                     vtkm::Int32 numOfValues,
                     vtkm::Int32 numOfPlanes,
                     bool cylindrical = false)
    : Portal(p)
    , NumberOfValues(numOfValues)
    , NumberOfPlanes(numOfPlanes)
    , UseCylindrical(cylindrical)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return ((NumberOfValues / 2) * static_cast<vtkm::Id>(NumberOfPlanes));
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id2 index) const;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Vec<ValueType, 6> GetWedge(const IndicesExtrude& index) const;

  PortalType Portal;
  vtkm::Int32 NumberOfValues;
  vtkm::Int32 NumberOfPlanes;
  bool UseCylindrical;
};
template <typename PortalType>
typename ArrayPortalExtrude<PortalType>::ValueType
ArrayPortalExtrude<PortalType>::ArrayPortalExtrude::Get(vtkm::Id index) const
{
  using CompType = typename ValueType::ComponentType;

  const vtkm::Id realIdx = (index * 2) % this->NumberOfValues;
  const vtkm::Id whichPlane = (index * 2) / this->NumberOfValues;
  const auto phi = static_cast<CompType>(whichPlane * (vtkm::TwoPi() / this->NumberOfPlanes));

  auto r = this->Portal.Get(realIdx);
  auto z = this->Portal.Get(realIdx + 1);
  if (this->UseCylindrical)
  {
    return ValueType(r, phi, z);
  }
  else
  {
    return ValueType(r * vtkm::Cos(phi), r * vtkm::Sin(phi), z);
  }
}

template <typename PortalType>
typename ArrayPortalExtrude<PortalType>::ValueType
ArrayPortalExtrude<PortalType>::ArrayPortalExtrude::Get(vtkm::Id2 index) const
{
  using CompType = typename ValueType::ComponentType;

  const vtkm::Id realIdx = (index[0] * 2);
  const vtkm::Id whichPlane = index[1];
  const auto phi = static_cast<CompType>(whichPlane * (vtkm::TwoPi() / this->NumberOfPlanes));

  auto r = this->Portal.Get(realIdx);
  auto z = this->Portal.Get(realIdx + 1);
  if (this->UseCylindrical)
  {
    return ValueType(r, phi, z);
  }
  else
  {
    return ValueType(r * vtkm::Cos(phi), r * vtkm::Sin(phi), z);
  }
}

template <typename PortalType>
vtkm::Vec<typename ArrayPortalExtrude<PortalType>::ValueType, 6>
ArrayPortalExtrude<PortalType>::ArrayPortalExtrude::GetWedge(const IndicesExtrude& index) const
{
  using CompType = typename ValueType::ComponentType;

  vtkm::Vec<ValueType, 6> result;
  for (int j = 0; j < 2; ++j)
  {
    const auto phi =
      static_cast<CompType>(index.Planes[j] * (vtkm::TwoPi() / this->NumberOfPlanes));
    for (int i = 0; i < 3; ++i)
    {
      const vtkm::Id realIdx = index.PointIds[j][i] * 2;
      auto r = this->Portal.Get(realIdx);
      auto z = this->Portal.Get(realIdx + 1);
      result[3 * j + i] = this->UseCylindrical
        ? ValueType(r, phi, z)
        : ValueType(r * vtkm::Cos(phi), r * vtkm::Sin(phi), z);
    }
  }

  return result;
}
}
}

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
  using BaseT = typename VecTraits<T>::BaseComponentType;
  using HandleType = vtkm::cont::ArrayHandle<BaseT>;
  using TPortalType = typename HandleType::PortalConstControl;

public:
  using ValueType = T;

  using PortalConstType = exec::ArrayPortalExtrude<TPortalType>;

  // Note that this array is read only, so you really should only be getting the const
  // version of the portal. If you actually try to write to this portal, you will
  // get an error.
  using PortalType = PortalConstType;

  Storage()
    : Array()
    , NumberOfPlanes(0)
  {
  }

  // Create with externally managed memory
  Storage(const BaseT* array, vtkm::Id arrayLength, vtkm::Int32 numberOfPlanes, bool cylindrical)
    : Array(vtkm::cont::make_ArrayHandle(array, arrayLength))
    , NumberOfPlanes(numberOfPlanes)
    , UseCylindrical(cylindrical)
  {
    VTKM_ASSERT(this->Array.GetNumberOfValues() >= 0);
  }

  Storage(const HandleType& array, vtkm::Int32 numberOfPlanes, bool cylindrical)
    : Array(array)
    , NumberOfPlanes(numberOfPlanes)
    , UseCylindrical(cylindrical)
  {
    VTKM_ASSERT(this->Array.GetNumberOfValues() >= 0);
  }

  PortalType GetPortal()
  {
    throw vtkm::cont::ErrorBadType(
      "Extrude ArrayHandles are read only. Cannot get writable portal.");
  }

  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Array.GetNumberOfValues() >= 0);
    return PortalConstType(this->Array.GetPortalConstControl(),
                           static_cast<vtkm::Int32>(this->Array.GetNumberOfValues()),
                           this->NumberOfPlanes,
                           this->UseCylindrical);
  }

  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Array.GetNumberOfValues() >= 0);
    return (this->Array.GetNumberOfValues() / 2) * static_cast<vtkm::Id>(this->NumberOfPlanes);
  }

  vtkm::Id GetLength() const { return this->Array.GetNumberOfValues(); }

  vtkm::Int32 GetNumberOfPlanes() const { return NumberOfPlanes; }

  bool GetUseCylindrical() const { return UseCylindrical; }
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

private:
  vtkm::Int32 NumberOfPlanes;
  bool UseCylindrical;
};

template <typename T, typename Device>
class VTKM_ALWAYS_EXPORT ArrayTransfer<T, internal::StorageTagExtrude, Device>
{
  using BaseT = typename VecTraits<T>::BaseComponentType;
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
    return PortalConstExecution(
      this->ControlData->Array.PrepareForInput(Device()),
      static_cast<vtkm::Int32>(this->ControlData->Array.GetNumberOfValues()),
      this->ControlData->GetNumberOfPlanes(),
      this->ControlData->GetUseCylindrical());
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
