//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_ExecutionWholeArray_h
#define vtk_m_exec_ExecutionWholeArray_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/exec/ExecutionObjectBase.h>

namespace vtkm
{
namespace exec
{

/// \c ExecutionWholeArray is an execution object that allows an array handle
/// content to be a parameter in an execution environment
/// function. This can be used to allow worklets to have a shared search
/// structure
///
template <typename T,
          typename StorageTag = VTKM_DEFAULT_STORAGE_TAG,
          typename DeviceAdapterTag = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class ExecutionWholeArray : public vtkm::exec::ExecutionObjectBase
{
public:
  using ValueType = T;
  using HandleType = vtkm::cont::ArrayHandle<T, StorageTag>;
  using PortalType = typename HandleType::template ExecutionTypes<DeviceAdapterTag>::Portal;

  VTKM_CONT
  ExecutionWholeArray()
    : Portal()
  {
  }

  VTKM_CONT
  ExecutionWholeArray(HandleType& handle)
    : Portal(handle.PrepareForInPlace(DeviceAdapterTag()))
  {
  }

  VTKM_CONT
  ExecutionWholeArray(HandleType& handle, vtkm::Id length)
    : Portal(handle.PrepareForOutput(length, DeviceAdapterTag()))
  {
  }

  VTKM_EXEC
  vtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VTKM_EXEC
  T Get(vtkm::Id index) const { return this->Portal.Get(index); }

  VTKM_EXEC
  T operator[](vtkm::Id index) const { return this->Portal.Get(index); }

  VTKM_EXEC
  void Set(vtkm::Id index, const T& t) const { this->Portal.Set(index, t); }

  VTKM_EXEC
  const PortalType& GetPortal() const { return this->Portal; }

private:
  PortalType Portal;
};

/// \c ExecutionWholeArrayConst is an execution object that allows an array handle
/// content to be a parameter in an execution environment
/// function. This can be used to allow worklets to have a shared search
/// structure
///
template <typename T,
          typename StorageTag = VTKM_DEFAULT_STORAGE_TAG,
          typename DeviceAdapterTag = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class ExecutionWholeArrayConst : public vtkm::exec::ExecutionObjectBase
{
public:
  using ValueType = T;
  using HandleType = vtkm::cont::ArrayHandle<T, StorageTag>;
  using PortalType = typename HandleType::template ExecutionTypes<DeviceAdapterTag>::PortalConst;

  VTKM_CONT
  ExecutionWholeArrayConst()
    : Portal()
  {
  }

  VTKM_CONT
  ExecutionWholeArrayConst(const HandleType& handle)
    : Portal(handle.PrepareForInput(DeviceAdapterTag()))
  {
  }

  VTKM_EXEC
  vtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VTKM_EXEC
  T Get(vtkm::Id index) const { return this->Portal.Get(index); }

  VTKM_EXEC
  T operator[](vtkm::Id index) const { return this->Portal.Get(index); }

  VTKM_EXEC
  const PortalType& GetPortal() const { return this->Portal; }

private:
  PortalType Portal;
};
}
} // namespace vtkm::exec

#endif //vtk_m_exec_ExecutionObjectBase_h
