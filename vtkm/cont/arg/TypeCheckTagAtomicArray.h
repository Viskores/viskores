//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_arg_TypeCheckTagAtomicArray_h
#define vtk_m_cont_arg_TypeCheckTagAtomicArray_h

#include <vtkm/cont/arg/TypeCheck.h>

#include <vtkm/ListTag.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageBasic.h>

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
template <typename TypeList = vtkm::cont::AtomicArrayTypeListTag>
struct TypeCheckTagAtomicArray
{
  VTKM_IS_LIST_TAG(TypeList);
};

template <typename TypeList, typename ArrayType>
struct TypeCheck<TypeCheckTagAtomicArray<TypeList>, ArrayType>
{
  static constexpr bool value = false;
};

template <typename T, typename TypeList>
struct TypeCheck<TypeCheckTagAtomicArray<TypeList>,
                 vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>>
{
  static constexpr bool value = (vtkm::ListContains<TypeList, T>::value &&
                                 vtkm::ListContains<vtkm::cont::AtomicArrayTypeListTag, T>::value);
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TypeCheckTagAtomicArray_h
