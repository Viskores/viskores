//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_cont_ArrayHandleConstant_h
#define vtk_m_cont_ArrayHandleConstant_h

#include <vtkm/cont/ArrayHandleImplicit.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

template <typename ValueType>
struct VTKM_ALWAYS_EXPORT ConstantFunctor
{
  VTKM_EXEC_CONT
  ConstantFunctor(const ValueType& value = ValueType())
    : Value(value)
  {
  }

  VTKM_EXEC_CONT
  ValueType operator()(vtkm::Id vtkmNotUsed(index)) const { return this->Value; }

private:
  ValueType Value;
};

} // namespace detail

/// \brief An array handle with a constant value.
///
/// ArrayHandleConstant is an implicit array handle with a constant value. A
/// constant array handle is constructed by giving a value and an array length.
/// The resulting array is of the given size with each entry the same value
/// given in the constructor. The array is defined implicitly, so there it
/// takes (almost) no memory.
///
template <typename T>
class ArrayHandleConstant : public vtkm::cont::ArrayHandleImplicit<detail::ConstantFunctor<T>>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleConstant,
                             (ArrayHandleConstant<T>),
                             (vtkm::cont::ArrayHandleImplicit<detail::ConstantFunctor<T>>));

  VTKM_CONT
  ArrayHandleConstant(T value, vtkm::Id numberOfValues = 0)
    : Superclass(detail::ConstantFunctor<T>(value), numberOfValues)
  {
  }
};

/// make_ArrayHandleConstant is convenience function to generate an
/// ArrayHandleImplicit.  It takes a functor and the virtual length of the
/// array.
///
template <typename T>
vtkm::cont::ArrayHandleConstant<T> make_ArrayHandleConstant(T value, vtkm::Id numberOfValues)
{
  return vtkm::cont::ArrayHandleConstant<T>(value, numberOfValues);
}
}
} // vtkm::cont

#endif //vtk_m_cont_ArrayHandleConstant_h
