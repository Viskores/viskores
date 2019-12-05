//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_AtomicArray_h
#define vtk_m_cont_AtomicArray_h

#include <vtkm/List.h>
#include <vtkm/ListTag.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/exec/AtomicArrayExecutionObject.h>

namespace vtkm
{
namespace cont
{

/// \brief A type list containing types that can be used with an AtomicArray.
///
using AtomicArrayTypeList = vtkm::List<vtkm::UInt32, vtkm::Int32, vtkm::UInt64, vtkm::Int64>;

struct VTKM_DEPRECATED(1.6,
                       "AtomicArrayTypeListTag replaced by AtomicArrayTypeList. Note that the "
                       "new AtomicArrayTypeList cannot be subclassed.") AtomicArrayTypeListTag
  : vtkm::internal::ListAsListTag<AtomicArrayTypeList>
{
};


/// A class that can be used to atomically operate on an array of values safely
/// across multiple instances of the same worklet. This is useful when you have
/// an algorithm that needs to accumulate values in parallel, but writing out a
/// value per worklet might be memory prohibitive.
///
/// To construct an AtomicArray you will need to pass in an
/// vtkm::cont::ArrayHandle that is used as the underlying storage for the
/// AtomicArray
///
/// Supported Operations: get / add / compare and swap (CAS). See
/// AtomicArrayExecutionObject for details.
///
/// Supported Types: 32 / 64 bit signed/unsigned integers.
///
///
template <typename T>
class AtomicArray : public vtkm::cont::ExecutionObjectBase
{
  static constexpr bool ValueTypeIsValid = vtkm::ListHas<AtomicArrayTypeList, T>::value;
  VTKM_STATIC_ASSERT_MSG(ValueTypeIsValid, "AtomicArray used with unsupported ValueType.");


public:
  using ValueType = T;

  VTKM_CONT
  AtomicArray()
    : Handle(vtkm::cont::ArrayHandle<T>())
  {
  }

  VTKM_CONT AtomicArray(vtkm::cont::ArrayHandle<T> handle)
    : Handle(handle)
  {
  }

  template <typename Device>
  VTKM_CONT vtkm::exec::AtomicArrayExecutionObject<T, Device> PrepareForExecution(Device) const
  {
    return vtkm::exec::AtomicArrayExecutionObject<T, Device>(this->Handle);
  }

private:
  vtkm::cont::ArrayHandle<T> Handle;
};
}
} // namespace vtkm::exec

#endif //vtk_m_cont_AtomicArray_h
