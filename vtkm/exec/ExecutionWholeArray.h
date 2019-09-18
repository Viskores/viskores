//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_ExecutionWholeArray_h
#define vtk_m_exec_ExecutionWholeArray_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapter.h>

namespace vtkm
{
namespace exec
{

/// The following classes have been deprecated and are meant to be used
/// internally only. Please use the \c WholeArrayIn, \c WholeArrayOut, and
/// \c WholeArrayInOut \c ControlSignature tags instead.

/// \c ExecutionWholeArray is an execution object that allows an array handle
/// content to be a parameter in an execution environment
/// function. This can be used to allow worklets to have a shared search
/// structure.
///
template <typename T, typename StorageTag, typename DeviceAdapterTag>
class ExecutionWholeArray
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
template <typename T, typename StorageTag, typename DeviceAdapterTag>
class ExecutionWholeArrayConst
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

#endif //vtk_m_exec_ExecutionWholeArray_h
