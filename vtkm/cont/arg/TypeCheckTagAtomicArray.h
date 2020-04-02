//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TypeCheckTagAtomicArray_h
#define vtk_m_cont_arg_TypeCheckTagAtomicArray_h

#include <vtkm/cont/arg/TypeCheck.h>

#include <vtkm/List.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageBasic.h>
#include <vtkm/cont/StorageVirtual.h>

#include <vtkm/cont/AtomicArray.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// The atomic array type check passes for an \c ArrayHandle of a structure
/// that is valid for atomic access. There are many restrictions on the
/// type of data that can be used for an atomic array.
///
struct TypeCheckTagAtomicArray;

template <typename ArrayType>
struct TypeCheck<TypeCheckTagAtomicArray, ArrayType>
{
  static constexpr bool value = false;
};

template <typename T>
struct TypeCheck<TypeCheckTagAtomicArray, vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>>
{
  static constexpr bool value = vtkm::ListHas<vtkm::cont::AtomicArrayTypeList, T>::value;
};

template <typename T>
struct TypeCheck<TypeCheckTagAtomicArray, vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagVirtual>>
{
  static constexpr bool value = vtkm::ListHas<vtkm::cont::AtomicArrayTypeList, T>::value;
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TypeCheckTagAtomicArray_h
