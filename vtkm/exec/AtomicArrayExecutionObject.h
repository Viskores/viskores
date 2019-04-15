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

#include <vtkm/ListTag.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapter.h>

namespace vtkm
{
namespace exec
{

template <typename T, typename Device>
class AtomicArrayExecutionObject
{
public:
  using ValueType = T;

  VTKM_CONT
  AtomicArrayExecutionObject()
    : AtomicImplementation((vtkm::cont::ArrayHandle<T>()))
  {
  }

  template <typename StorageType>
  VTKM_CONT AtomicArrayExecutionObject(vtkm::cont::ArrayHandle<T, StorageType> handle)
    : AtomicImplementation(handle)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  T Add(vtkm::Id index, const T& value) const
  {
    return this->AtomicImplementation.Add(index, value);
  }

  //
  // Compare and Swap is an atomic exchange operation. If the value at
  // the index is equal to oldValue, then newValue is written to the index.
  // The operation was successful if return value is equal to oldValue
  //
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  T CompareAndSwap(vtkm::Id index, const T& newValue, const T& oldValue) const
  {
    return this->AtomicImplementation.CompareAndSwap(index, newValue, oldValue);
  }

private:
  vtkm::cont::DeviceAdapterAtomicArrayImplementation<T, Device> AtomicImplementation;
};
}
} // namespace vtkm::exec

#endif //vtk_m_exec_AtomicArrayExecutionObject_h
