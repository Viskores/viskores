//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
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

//=============================================================================
// Specializations of serialization related classes
namespace vtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<vtkm::cont::detail::ConstantFunctor<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_ConstantFunctor<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandleConstant<T>>
  : SerializableTypeString<vtkm::cont::ArrayHandleImplicit<vtkm::cont::detail::ConstantFunctor<T>>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleConstant<T>>
  : Serialization<vtkm::cont::ArrayHandleImplicit<vtkm::cont::detail::ConstantFunctor<T>>>
{
};

} // diy

#endif //vtk_m_cont_ArrayHandleConstant_h
