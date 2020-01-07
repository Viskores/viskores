//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleCounting_h
#define vtk_m_cont_ArrayHandleCounting_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageImplicit.h>

#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace cont
{

struct VTKM_ALWAYS_EXPORT StorageTagCounting
{
};

namespace internal
{

/// \brief An implicit array portal that returns an counting value.
template <class CountingValueType>
class VTKM_ALWAYS_EXPORT ArrayPortalCounting
{
  using ComponentType = typename vtkm::VecTraits<CountingValueType>::ComponentType;

public:
  using ValueType = CountingValueType;

  VTKM_EXEC_CONT
  ArrayPortalCounting()
    : Start(0)
    , Step(1)
    , NumberOfValues(0)
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalCounting(ValueType start, ValueType step, vtkm::Id numValues)
    : Start(start)
    , Step(step)
    , NumberOfValues(numValues)
  {
  }

  template <typename OtherValueType>
  VTKM_EXEC_CONT ArrayPortalCounting(const ArrayPortalCounting<OtherValueType>& src)
    : Start(src.Start)
    , Step(src.Step)
    , NumberOfValues(src.NumberOfValues)
  {
  }

  template <typename OtherValueType>
  VTKM_EXEC_CONT ArrayPortalCounting<ValueType>& operator=(
    const ArrayPortalCounting<OtherValueType>& src)
  {
    this->Start = src.Start;
    this->Step = src.Step;
    this->NumberOfValues = src.NumberOfValues;
    return *this;
  }

  VTKM_EXEC_CONT
  ValueType GetStart() const { return this->Start; }

  VTKM_EXEC_CONT
  ValueType GetStep() const { return this->Step; }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    return ValueType(this->Start + this->Step * ValueType(static_cast<ComponentType>(index)));
  }

private:
  ValueType Start;
  ValueType Step;
  vtkm::Id NumberOfValues;
};

template <typename T>
using StorageTagCountingSuperclass =
  vtkm::cont::StorageTagImplicit<internal::ArrayPortalCounting<T>>;

template <typename T>
struct Storage<T, vtkm::cont::StorageTagCounting> : Storage<T, StorageTagCountingSuperclass<T>>
{
  using Superclass = Storage<T, StorageTagCountingSuperclass<T>>;
  using Superclass::Superclass;
};

template <typename T, typename Device>
struct ArrayTransfer<T, vtkm::cont::StorageTagCounting, Device>
  : ArrayTransfer<T, StorageTagCountingSuperclass<T>, Device>
{
  using Superclass = ArrayTransfer<T, StorageTagCountingSuperclass<T>, Device>;
  using Superclass::Superclass;
};

} // namespace internal

/// ArrayHandleCounting is a specialization of ArrayHandle. By default it
/// contains a increment value, that is increment for each step between zero
/// and the passed in length
template <typename CountingValueType>
class ArrayHandleCounting
  : public vtkm::cont::ArrayHandle<CountingValueType, vtkm::cont::StorageTagCounting>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleCounting,
                             (ArrayHandleCounting<CountingValueType>),
                             (vtkm::cont::ArrayHandle<CountingValueType, StorageTagCounting>));

  VTKM_CONT
  ArrayHandleCounting(CountingValueType start, CountingValueType step, vtkm::Id length)
    : Superclass(typename Superclass::PortalConstControl(start, step, length))
  {
  }
};

/// A convenience function for creating an ArrayHandleCounting. It takes the
/// value to start counting from and and the number of times to increment.
template <typename CountingValueType>
VTKM_CONT vtkm::cont::ArrayHandleCounting<CountingValueType>
make_ArrayHandleCounting(CountingValueType start, CountingValueType step, vtkm::Id length)
{
  return vtkm::cont::ArrayHandleCounting<CountingValueType>(start, step, length);
}
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandleCounting<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Counting<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagCounting>>
  : SerializableTypeString<vtkm::cont::ArrayHandleCounting<T>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleCounting<T>>
{
private:
  using Type = vtkm::cont::ArrayHandleCounting<T>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto portal = obj.GetPortalConstControl();
    vtkmdiy::save(bb, portal.GetStart());
    vtkmdiy::save(bb, portal.GetStep());
    vtkmdiy::save(bb, portal.GetNumberOfValues());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    T start{}, step{};
    vtkm::Id count = 0;

    vtkmdiy::load(bb, start);
    vtkmdiy::load(bb, step);
    vtkmdiy::load(bb, count);

    obj = vtkm::cont::make_ArrayHandleCounting(start, step, count);
  }
};

template <typename T>
struct Serialization<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagCounting>>
  : Serialization<vtkm::cont::ArrayHandleCounting<T>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleCounting_h
