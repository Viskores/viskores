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

#include <vtkm/Deprecated.h>

namespace vtkm
{
namespace exec
{

/// The following classes have been sort of deprecated and are meant to be used
/// internally only. Please use the \c WholeArrayIn, \c WholeArrayOut, and
/// \c WholeArrayInOut \c ControlSignature tags instead.

/// \c ExecutionWholeArray is an execution object that allows an array handle
/// content to be a parameter in an execution environment
/// function. This can be used to allow worklets to have a shared search
/// structure.
///
template <typename T, typename StorageTag, typename... MaybeDevice>
class ExecutionWholeArray;

template <typename T, typename StorageTag>
class ExecutionWholeArray<T, StorageTag>
{
public:
  using ValueType = T;
  using HandleType = vtkm::cont::ArrayHandle<T, StorageTag>;
  using PortalType = typename HandleType::WritePortalType;

  VTKM_CONT
  ExecutionWholeArray()
    : Portal()
  {
  }

  VTKM_CONT
  ExecutionWholeArray(HandleType& handle,
                      vtkm::cont::DeviceAdapterId device,
                      vtkm::cont::Token& token)
    : Portal(handle.PrepareForInPlace(device, token))
  {
  }

  VTKM_CONT
  ExecutionWholeArray(HandleType& handle,
                      vtkm::Id length,
                      vtkm::cont::DeviceAdapterId device,
                      vtkm::cont::Token& token)
    : Portal(handle.PrepareForOutput(length, device, token))
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

template <typename T, typename StorageTag, typename Device>
class VTKM_DEPRECATED(1.6, "ExecutionWholeArray no longer uses Device template parameter.")
  ExecutionWholeArray<T, StorageTag, Device> : public ExecutionWholeArray<T, StorageTag>
{
  using Superclass = ExecutionWholeArray<T, StorageTag>;
  using HandleType = typename Superclass::HandleType;

public:
  using Superclass::Superclass;

  VTKM_CONT ExecutionWholeArray(HandleType& handle)
    : Superclass(handle, Device{}, vtkm::cont::Token{})
  {
  }

  VTKM_CONT
  ExecutionWholeArray(HandleType& handle, vtkm::Id length)
    : Superclass(handle, length, Device{}, vtkm::cont::Token{})
  {
  }

  VTKM_CONT
  ExecutionWholeArray(HandleType& handle, vtkm::cont::Token& token)
    : Superclass(handle, Device{}, token)
  {
  }

  VTKM_CONT
  ExecutionWholeArray(HandleType& handle, vtkm::Id length, vtkm::cont::Token& token)
    : Superclass(handle, length, Device{}, token)
  {
  }
};

/// \c ExecutionWholeArrayConst is an execution object that allows an array handle
/// content to be a parameter in an execution environment
/// function. This can be used to allow worklets to have a shared search
/// structure
///
template <typename T, typename StorageTag, typename... MaybeDevice>
class ExecutionWholeArrayConst;

template <typename T, typename StorageTag>
class ExecutionWholeArrayConst<T, StorageTag>
{
public:
  using ValueType = T;
  using HandleType = vtkm::cont::ArrayHandle<T, StorageTag>;
  using PortalType = typename HandleType::ReadPortalType;

  VTKM_CONT
  ExecutionWholeArrayConst()
    : Portal()
  {
  }

  VTKM_CONT
  ExecutionWholeArrayConst(const HandleType& handle,
                           vtkm::cont::DeviceAdapterId device,
                           vtkm::cont::Token& token)
    : Portal(handle.PrepareForInput(device, token))
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

template <typename T, typename StorageTag, typename Device>
class VTKM_DEPRECATED(1.6, "ExecutionWholeArray no longer uses Device template parameter.")
  ExecutionWholeArrayConst<T, StorageTag, Device> : public ExecutionWholeArrayConst<T, StorageTag>
{
  using Superclass = ExecutionWholeArrayConst<T, StorageTag>;
  using HandleType = typename Superclass::HandleType;

public:
  using Superclass::Superclass;

  VTKM_CONT ExecutionWholeArrayConst(HandleType& handle)
    : Superclass(handle, Device{}, vtkm::cont::Token{})
  {
  }

  VTKM_CONT
  ExecutionWholeArrayConst(HandleType& handle, vtkm::cont::Token& token)
    : Superclass(handle, Device{}, token)
  {
  }
};


}
} // namespace vtkm::exec

#endif //vtk_m_exec_ExecutionWholeArray_h
