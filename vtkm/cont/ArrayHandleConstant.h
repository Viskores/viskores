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

struct VTKM_ALWAYS_EXPORT StorageTagConstant
{
};

namespace internal
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

template <typename T>
using StorageTagConstantSuperclass =
  typename vtkm::cont::ArrayHandleImplicit<ConstantFunctor<T>>::StorageTag;

template <typename T>
struct Storage<T, vtkm::cont::StorageTagConstant> : Storage<T, StorageTagConstantSuperclass<T>>
{
  using Superclass = Storage<T, StorageTagConstantSuperclass<T>>;
  using Superclass::Superclass;
};

template <typename T, typename Device>
struct ArrayTransfer<T, vtkm::cont::StorageTagConstant, Device>
  : ArrayTransfer<T, StorageTagConstantSuperclass<T>, Device>
{
  using Superclass = ArrayTransfer<T, StorageTagConstantSuperclass<T>, Device>;
  using Superclass::Superclass;
};

} // namespace internal

/// \brief An array handle with a constant value.
///
/// ArrayHandleConstant is an implicit array handle with a constant value. A
/// constant array handle is constructed by giving a value and an array length.
/// The resulting array is of the given size with each entry the same value
/// given in the constructor. The array is defined implicitly, so there it
/// takes (almost) no memory.
///
template <typename T>
class ArrayHandleConstant : public vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConstant>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleConstant,
                             (ArrayHandleConstant<T>),
                             (vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConstant>));

  VTKM_CONT
  ArrayHandleConstant(T value, vtkm::Id numberOfValues = 0)
    : Superclass(typename Superclass::PortalConstControl(internal::ConstantFunctor<T>(value),
                                                         numberOfValues))
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
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandleConstant<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Constant<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConstant>>
  : SerializableTypeString<vtkm::cont::ArrayHandleConstant<T>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleConstant<T>>
{
private:
  using Type = vtkm::cont::ArrayHandleConstant<T>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, obj.GetNumberOfValues());
    vtkmdiy::save(bb, obj.GetPortalConstControl().Get(0));
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    vtkm::Id count = 0;
    vtkmdiy::load(bb, count);

    T value;
    vtkmdiy::load(bb, value);

    obj = vtkm::cont::make_ArrayHandleConstant(value, count);
  }
};

template <typename T>
struct Serialization<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConstant>>
  : Serialization<vtkm::cont::ArrayHandleConstant<T>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleConstant_h
