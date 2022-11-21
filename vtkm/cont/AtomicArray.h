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
/// @cond NONE
using AtomicArrayTypeList =
  vtkm::List<vtkm::UInt32, vtkm::Int32, vtkm::UInt64, vtkm::Int64, vtkm::Float32, vtkm::Float64>;
/// @endcond


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

  VTKM_CONT vtkm::exec::AtomicArrayExecutionObject<T> PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    return vtkm::exec::AtomicArrayExecutionObject<T>(this->Handle, device, token);
  }

private:
  /// @cond NONE
  vtkm::cont::ArrayHandle<T> Handle;
  /// @endcond
};
}
} // namespace vtkm::exec

#endif //vtk_m_cont_AtomicArray_h
