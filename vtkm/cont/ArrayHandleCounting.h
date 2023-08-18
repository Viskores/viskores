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

#include <vtkm/cont/ArrayHandleImplicit.h>

#include <vtkm/cont/internal/ArrayRangeComputeUtils.h>

#include <vtkm/Range.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/VecFlat.h>
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

namespace detail
{

template <typename T>
struct CanCountImpl
{
  using VTraits = vtkm::VecTraits<T>;
  using BaseType = typename VTraits::BaseComponentType;
  using TTraits = vtkm::TypeTraits<BaseType>;
  static constexpr bool IsNumeric =
    !std::is_same<typename TTraits::NumericTag, vtkm::TypeTraitsUnknownTag>::value;
  static constexpr bool IsBool = std::is_same<BaseType, bool>::value;

  static constexpr bool value = IsNumeric && !IsBool;
};

} // namespace detail

// Not all types can be counted.
template <typename T>
struct CanCount
{
  static constexpr bool value = detail::CanCountImpl<T>::value;
};

template <typename T>
using StorageTagCountingSuperclass =
  vtkm::cont::StorageTagImplicit<internal::ArrayPortalCounting<T>>;

template <typename T>
struct Storage<T, typename std::enable_if<CanCount<T>::value, vtkm::cont::StorageTagCounting>::type>
  : Storage<T, StorageTagCountingSuperclass<T>>
{
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
    : Superclass(internal::PortalToArrayHandleImplicitBuffers(
        internal::ArrayPortalCounting<CountingValueType>(start, step, length)))
  {
  }

  VTKM_CONT CountingValueType GetStart() const { return this->ReadPortal().GetStart(); }

  VTKM_CONT CountingValueType GetStep() const { return this->ReadPortal().GetStep(); }
};

/// A convenience function for creating an ArrayHandleCounting. It takes the
/// value to start counting from and and the number of times to increment.
template <typename CountingValueType>
VTKM_CONT vtkm::cont::ArrayHandleCounting<CountingValueType>
make_ArrayHandleCounting(CountingValueType start, CountingValueType step, vtkm::Id length)
{
  return vtkm::cont::ArrayHandleCounting<CountingValueType>(start, step, length);
}

namespace internal
{

template <typename S>
struct ArrayRangeComputeImpl;

template <>
struct VTKM_CONT_EXPORT ArrayRangeComputeImpl<vtkm::cont::StorageTagCounting>
{
  template <typename T>
  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> operator()(
    const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagCounting>& input,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
    bool vtkmNotUsed(computeFiniteRange), // assume array produces only finite values
    vtkm::cont::DeviceAdapterId device) const
  {
    using Traits = vtkm::VecTraits<vtkm::VecFlat<T>>;
    vtkm::cont::ArrayHandle<vtkm::Range> result;
    result.Allocate(Traits::NUM_COMPONENTS);

    if (input.GetNumberOfValues() <= 0)
    {
      result.Fill(vtkm::Range{});
      return result;
    }

    vtkm::Id2 firstAndLast{ 0, input.GetNumberOfValues() - 1 };
    if (maskArray.GetNumberOfValues() > 0)
    {
      firstAndLast = GetFirstAndLastUnmaskedIndices(maskArray, device);
    }

    if (firstAndLast[1] < firstAndLast[0])
    {
      result.Fill(vtkm::Range{});
      return result;
    }

    auto portal = result.WritePortal();
    // assume the values to be finite
    auto first = make_VecFlat(input.ReadPortal().Get(firstAndLast[0]));
    auto last = make_VecFlat(input.ReadPortal().Get(firstAndLast[1]));
    for (vtkm::IdComponent cIndex = 0; cIndex < Traits::NUM_COMPONENTS; ++cIndex)
    {
      auto firstComponent = Traits::GetComponent(first, cIndex);
      auto lastComponent = Traits::GetComponent(last, cIndex);
      portal.Set(cIndex,
                 vtkm::Range(vtkm::Min(firstComponent, lastComponent),
                             vtkm::Max(firstComponent, lastComponent)));
    }

    return result;
  }
};

} // namespace internal

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
    auto portal = obj.ReadPortal();
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
