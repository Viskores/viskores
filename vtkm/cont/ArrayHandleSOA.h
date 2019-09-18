//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleSOA_h
#define vtk_m_cont_ArrayHandleSOA_h

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/Math.h>
#include <vtkm/VecTraits.h>

#include <vtkm/internal/ArrayPortalHelpers.h>

#include <vtkmtaotuple/include/tao/seq/make_integer_sequence.hpp>

#include <array>
#include <limits>
#include <type_traits>

namespace vtkm
{

namespace internal
{

/// \brief An array portal that combines indices from multiple sources.
///
/// This will only work if \c VecTraits is defined for the type.
///
template <typename ValueType_, typename SourcePortalType>
class ArrayPortalSOA
{
public:
  using ValueType = ValueType_;

private:
  using ComponentType = typename SourcePortalType::ValueType;

  VTKM_STATIC_ASSERT(vtkm::HasVecTraits<ValueType>::value);
  using VTraits = vtkm::VecTraits<ValueType>;
  VTKM_STATIC_ASSERT((std::is_same<typename VTraits::ComponentType, ComponentType>::value));
  static constexpr vtkm::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

  SourcePortalType Portals[NUM_COMPONENTS];
  vtkm::Id NumberOfValues;

public:
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT explicit ArrayPortalSOA(vtkm::Id numValues = 0)
    : NumberOfValues(numValues)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT void SetPortal(vtkm::IdComponent index, const SourcePortalType& portal)
  {
    this->Portals[index] = portal;
  }

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  template <typename SPT = SourcePortalType,
            typename Supported = typename vtkm::internal::PortalSupportsGets<SPT>::type,
            typename = typename std::enable_if<Supported::value>::type>
  VTKM_EXEC_CONT ValueType Get(vtkm::Id valueIndex) const
  {
    return this->Get(valueIndex, tao::seq::make_index_sequence<NUM_COMPONENTS>());
  }

  template <typename SPT = SourcePortalType,
            typename Supported = typename vtkm::internal::PortalSupportsSets<SPT>::type,
            typename = typename std::enable_if<Supported::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id valueIndex, const ValueType& value) const
  {
    this->Set(valueIndex, value, tao::seq::make_index_sequence<NUM_COMPONENTS>());
  }

private:
  template <std::size_t I>
  VTKM_EXEC_CONT ComponentType GetComponent(vtkm::Id valueIndex) const
  {
    return this->Portals[I].Get(valueIndex);
  }

  template <std::size_t... I>
  VTKM_EXEC_CONT ValueType Get(vtkm::Id valueIndex, tao::seq::index_sequence<I...>) const
  {
    return ValueType{ this->GetComponent<I>(valueIndex)... };
  }

  template <std::size_t I>
  VTKM_EXEC_CONT bool SetComponent(vtkm::Id valueIndex, const ValueType& value) const
  {
    this->Portals[I].Set(valueIndex,
                         VTraits::GetComponent(value, static_cast<vtkm::IdComponent>(I)));
    return true;
  }

  template <std::size_t... I>
  VTKM_EXEC_CONT void Set(vtkm::Id valueIndex,
                          const ValueType& value,
                          tao::seq::index_sequence<I...>) const
  {
    // Is there a better way to unpack an expression and execute them with no other side effects?
    (void)std::initializer_list<bool>{ this->SetComponent<I>(valueIndex, value)... };
  }
};

} // namespace internal

namespace cont
{

struct VTKM_ALWAYS_EXPORT StorageTagSOA
{
};

namespace internal
{

namespace detail
{

template <typename ValueType, typename PortalType, typename IsTrueVec>
struct SOAPortalChooser;

template <typename ValueType, typename PortalType>
struct SOAPortalChooser<ValueType, PortalType, std::true_type>
{
  using Type = vtkm::internal::ArrayPortalSOA<ValueType, PortalType>;
};

template <typename ValueType, typename PortalType>
struct SOAPortalChooser<ValueType, PortalType, std::false_type>
{
  using Type = PortalType;
};

template <typename ReturnType, typename ValueType, std::size_t NUM_COMPONENTS, typename PortalMaker>
ReturnType MakeSOAPortal(std::array<vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagBasic>,
                                    NUM_COMPONENTS> arrays,
                         vtkm::Id numValues,
                         const PortalMaker& portalMaker)
{
  ReturnType portal(numValues);
  for (std::size_t componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
  {
    portal.SetPortal(static_cast<vtkm::IdComponent>(componentIndex),
                     portalMaker(arrays[componentIndex]));
    VTKM_ASSERT(arrays[componentIndex].GetNumberOfValues() == numValues);
  }
  return portal;
}

template <typename ReturnType, typename ValueType, typename PortalMaker>
ReturnType MakeSOAPortal(
  std::array<vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagBasic>, 1> arrays,
  vtkm::Id vtkmNotUsed(numValues),
  const PortalMaker& portalMaker)
{
  return portalMaker(arrays[0]);
}

} // namespace detail

template <typename ValueType>
struct ArrayHandleSOATraits
{
  using VTraits = vtkm::VecTraits<ValueType>;
  using ComponentType = typename VTraits::ComponentType;
  using BaseArrayType = vtkm::cont::ArrayHandle<ComponentType, vtkm::cont::StorageTagBasic>;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;
  VTKM_STATIC_ASSERT_MSG(NUM_COMPONENTS > 0,
                         "ArrayHandleSOA requires a type with at least 1 component.");

  using IsTrueVec = std::integral_constant<bool, (NUM_COMPONENTS > 1)>;

  using PortalControl = typename detail::SOAPortalChooser<ValueType,
                                                          typename BaseArrayType::PortalControl,
                                                          IsTrueVec>::Type;
  using PortalConstControl =
    typename detail::SOAPortalChooser<ValueType,
                                      typename BaseArrayType::PortalConstControl,
                                      IsTrueVec>::Type;

  template <typename Device>
  using PortalExecution = typename detail::SOAPortalChooser<
    ValueType,
    typename BaseArrayType::template ExecutionTypes<Device>::Portal,
    IsTrueVec>::Type;
  template <typename Device>
  using PortalConstExecution = typename detail::SOAPortalChooser<
    ValueType,
    typename BaseArrayType::template ExecutionTypes<Device>::PortalConst,
    IsTrueVec>::Type;
};

template <typename ValueType_>
class Storage<ValueType_, vtkm::cont::StorageTagSOA>
{
  using Traits = ArrayHandleSOATraits<ValueType_>;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = Traits::NUM_COMPONENTS;
  using BaseArrayType = typename Traits::BaseArrayType;

  std::array<BaseArrayType, NUM_COMPONENTS> Arrays;

  VTKM_CONT bool IsValidImpl(std::true_type) const
  {
    vtkm::Id size = this->Arrays[0].GetNumberOfValues();
    for (vtkm::IdComponent componentIndex = 1; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      if (this->GetArray(componentIndex).GetNumberOfValues() != size)
      {
        return false;
      }
    }
    return true;
  }

  VTKM_CONT constexpr bool IsValidImpl(std::false_type) const { return true; }

public:
  using ValueType = ValueType_;
  using PortalType = typename Traits::PortalControl;
  using PortalConstType = typename Traits::PortalConstControl;

  VTKM_CONT bool IsValid() const { return this->IsValidImpl(typename Traits::IsTrueVec{}); }

  Storage() = default;

  VTKM_CONT Storage(std::initializer_list<BaseArrayType>&& arrays)
    : Arrays{ std::move(arrays) }
  {
    VTKM_ASSERT(IsValid());
  }

  // For this constructor to work, all types have to be
  // vtkm::cont::ArrayHandle<ValueType, StorageTagBasic>
  template <typename... ArrayTypes>
  VTKM_CONT Storage(const BaseArrayType& array0, const ArrayTypes&... arrays)
    : Arrays{ { array0, arrays... } }
  {
    VTKM_ASSERT(IsValid());
  }

  VTKM_CONT BaseArrayType& GetArray(vtkm::IdComponent index)
  {
    return this->Arrays[static_cast<std::size_t>(index)];
  }

  VTKM_CONT const BaseArrayType& GetArray(vtkm::IdComponent index) const
  {
    return this->Arrays[static_cast<std::size_t>(index)];
  }

  VTKM_CONT std::array<BaseArrayType, NUM_COMPONENTS>& GetArrays() { return this->Arrays; }

  VTKM_CONT const std::array<BaseArrayType, NUM_COMPONENTS>& GetArrays() const
  {
    return this->Arrays;
  }

  VTKM_CONT void SetArray(vtkm::IdComponent index, const BaseArrayType& array)
  {
    this->Arrays[static_cast<std::size_t>(index)] = array;
  }

  VTKM_CONT vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(IsValid());
    return this->GetArray(0).GetNumberOfValues();
  }

  VTKM_CONT PortalType GetPortal()
  {
    VTKM_ASSERT(this->IsValid());
    return detail::MakeSOAPortal<PortalType>(
      this->Arrays, this->GetNumberOfValues(), [](BaseArrayType& array) {
        return array.GetPortalControl();
      });
  }

  VTKM_CONT PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->IsValid());
    return detail::MakeSOAPortal<PortalConstType>(
      this->Arrays, this->GetNumberOfValues(), [](const BaseArrayType& array) {
        return array.GetPortalConstControl();
      });
  }

  VTKM_CONT void Allocate(vtkm::Id numValues)
  {
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      this->GetArray(componentIndex).Allocate(numValues);
    }
  }

  VTKM_CONT void Shrink(vtkm::Id numValues)
  {
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      this->GetArray(componentIndex).Shrink(numValues);
    }
  }

  VTKM_CONT void ReleaseResources()
  {
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      this->GetArray(componentIndex).ReleaseResources();
    }
  }
};

template <typename ValueType_, typename Device>
class ArrayTransfer<ValueType_, vtkm::cont::StorageTagSOA, Device>
{
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  using StorageType = vtkm::cont::internal::Storage<ValueType_, vtkm::cont::StorageTagSOA>;

  using Traits = ArrayHandleSOATraits<ValueType_>;
  using BaseArrayType = typename Traits::BaseArrayType;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = Traits::NUM_COMPONENTS;

  StorageType* Storage;

public:
  using ValueType = ValueType_;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = typename Traits::template PortalExecution<Device>;
  using PortalConstExecution = typename Traits::template PortalConstExecution<Device>;

  VTKM_CONT ArrayTransfer(StorageType* storage)
    : Storage(storage)
  {
  }

  VTKM_CONT vtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  VTKM_CONT PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData)) const
  {
    return detail::MakeSOAPortal<PortalConstExecution>(
      this->Storage->GetArrays(), this->GetNumberOfValues(), [](const BaseArrayType& array) {
        return array.PrepareForInput(Device{});
      });
  }

  VTKM_CONT PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData)) const
  {
    return detail::MakeSOAPortal<PortalExecution>(
      this->Storage->GetArrays(), this->GetNumberOfValues(), [](BaseArrayType& array) {
        return array.PrepareForInPlace(Device{});
      });
  }

  VTKM_CONT PortalExecution PrepareForOutput(vtkm::Id numValues) const
  {
    return detail::MakeSOAPortal<PortalExecution>(
      this->Storage->GetArrays(), numValues, [numValues](BaseArrayType& array) {
        return array.PrepareForOutput(numValues, Device{});
      });
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handle should automatically retrieve the output data as
    // necessary.
  }

  VTKM_CONT void Shrink(vtkm::Id numValues)
  {
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      this->Storage->GetArray(componentIndex).Shrink(numValues);
    }
  }

  VTKM_CONT void ReleaseResources()
  {
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      this->Storage->GetArray(componentIndex).ReleaseResourcesExecution();
    }
  }
};

} // namespace internal

/// \brief An \c ArrayHandle that for Vecs stores each component in a separate physical array.
///
/// \c ArrayHandleSOA behaves like a regular \c ArrayHandle (with a basic storage) except that
/// if you specify a \c ValueType of a \c Vec or a \c Vec-like, it will actually store each
/// component in a separate physical array. When data are retrieved from the array, they are
/// reconstructed into \c Vec objects as expected.
///
/// The intention of this array type is to help cover the most common ways data is lain out in
/// memory. Typically, arrays of data are either an "array of structures" like the basic storage
/// where you have a single array of structures (like \c Vec) or a "structure of arrays" where
/// you have an array of a basic type (like \c float) for each component of the data being
/// represented. The\c ArrayHandleSOA makes it easy to cover this second case without creating
/// special types.
///
/// \c ArrayHandleSOA can be constructed from a collection of \c ArrayHandle with basic storage.
/// This allows you to construct \c Vec arrays from components without deep copies.
///
template <typename ValueType_>
class ArrayHandleSOA : public ArrayHandle<ValueType_, vtkm::cont::StorageTagSOA>
{
  using Traits = vtkm::cont::internal::ArrayHandleSOATraits<ValueType_>;
  using ComponentType = typename Traits::ComponentType;
  using BaseArrayType = typename Traits::BaseArrayType;

  using StorageType = vtkm::cont::internal::Storage<ValueType_, vtkm::cont::StorageTagSOA>;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleSOA,
                             (ArrayHandleSOA<ValueType_>),
                             (ArrayHandle<ValueType_, vtkm::cont::StorageTagSOA>));

  ArrayHandleSOA(std::initializer_list<BaseArrayType>&& componentArrays)
    : Superclass(StorageType(std::move(componentArrays)))
  {
  }

  ArrayHandleSOA(std::initializer_list<std::vector<ComponentType>>&& componentVectors)
  {
    VTKM_ASSERT(componentVectors.size() == Traits::NUM_COMPONENTS);
    vtkm::IdComponent componentIndex = 0;
    for (auto&& vectorIter = componentVectors.begin(); vectorIter != componentVectors.end();
         ++vectorIter)
    {
      // Note, std::vectors that come from std::initializer_list must be copied because the scope
      // of the objects in the initializer list disappears.
      this->SetArray(componentIndex, vtkm::cont::make_ArrayHandle(*vectorIter, vtkm::CopyFlag::On));
      ++componentIndex;
    }
  }

  // This only works if all the templated arguments are of type std::vector<ComponentType>.
  template <typename... RemainingVectors>
  ArrayHandleSOA(vtkm::CopyFlag copy,
                 const std::vector<ComponentType>& vector0,
                 const RemainingVectors&... componentVectors)
    : Superclass(StorageType(vtkm::cont::make_ArrayHandle(vector0, copy),
                             vtkm::cont::make_ArrayHandle(componentVectors, copy)...))
  {
    VTKM_STATIC_ASSERT(sizeof...(RemainingVectors) + 1 == Traits::NUM_COMPONENTS);
  }

  // This only works if all the templated arguments are of type std::vector<ComponentType>.
  template <typename... RemainingVectors>
  ArrayHandleSOA(const std::vector<ComponentType>& vector0,
                 const RemainingVectors&... componentVectors)
    : Superclass(StorageType(vtkm::cont::make_ArrayHandle(vector0),
                             vtkm::cont::make_ArrayHandle(componentVectors)...))
  {
    VTKM_STATIC_ASSERT(sizeof...(RemainingVectors) + 1 == Traits::NUM_COMPONENTS);
  }

  ArrayHandleSOA(std::initializer_list<const ComponentType*> componentArrays,
                 vtkm::Id length,
                 vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
  {
    VTKM_ASSERT(componentArrays.size() == Traits::NUM_COMPONENTS);
    vtkm::IdComponent componentIndex = 0;
    for (auto&& vectorIter = componentArrays.begin(); vectorIter != componentArrays.end();
         ++vectorIter)
    {
      this->SetArray(componentIndex, vtkm::cont::make_ArrayHandle(*vectorIter, length, copy));
      ++componentIndex;
    }
  }

  // This only works if all the templated arguments are of type std::vector<ComponentType>.
  template <typename... RemainingArrays>
  ArrayHandleSOA(vtkm::Id length,
                 vtkm::CopyFlag copy,
                 const ComponentType* array0,
                 const RemainingArrays&... componentArrays)
    : Superclass(StorageType(vtkm::cont::make_ArrayHandle(array0, length, copy),
                             vtkm::cont::make_ArrayHandle(componentArrays, length, copy)...))
  {
    VTKM_STATIC_ASSERT(sizeof...(RemainingArrays) + 1 == Traits::NUM_COMPONENTS);
  }

  // This only works if all the templated arguments are of type std::vector<ComponentType>.
  template <typename... RemainingArrays>
  ArrayHandleSOA(vtkm::Id length,
                 const ComponentType* array0,
                 const RemainingArrays&... componentArrays)
    : Superclass(StorageType(vtkm::cont::make_ArrayHandle(array0, length),
                             vtkm::cont::make_ArrayHandle(componentArrays, length)...))
  {
    VTKM_STATIC_ASSERT(sizeof...(RemainingArrays) + 1 == Traits::NUM_COMPONENTS);
  }

  VTKM_CONT BaseArrayType& GetArray(vtkm::IdComponent index)
  {
    return this->GetStorage().GetArray(index);
  }

  VTKM_CONT const BaseArrayType& GetArray(vtkm::IdComponent index) const
  {
    return this->GetStorage().GetArray(index);
  }

  VTKM_CONT void SetArray(vtkm::IdComponent index, const BaseArrayType& array)
  {
    this->GetStorage().SetArray(index, array);
  }
};

template <typename ValueType>
VTKM_CONT ArrayHandleSOA<ValueType> make_ArrayHandleSOA(
  std::initializer_list<vtkm::cont::ArrayHandle<typename vtkm::VecTraits<ValueType>::ComponentType,
                                                vtkm::cont::StorageTagBasic>>&& componentArrays)
{
  return ArrayHandleSOA<ValueType>(std::move(componentArrays));
}

template <typename ComponentType, typename... RemainingArrays>
VTKM_CONT
  ArrayHandleSOA<vtkm::Vec<ComponentType, vtkm::IdComponent(sizeof...(RemainingArrays) + 1)>>
  make_ArrayHandleSOA(
    const vtkm::cont::ArrayHandle<ComponentType, vtkm::cont::StorageTagBasic>& componentArray0,
    const RemainingArrays&... componentArrays)
{
  return { componentArray0, componentArrays... };
}

template <typename ValueType>
VTKM_CONT ArrayHandleSOA<ValueType> make_ArrayHandleSOA(
  std::initializer_list<std::vector<typename vtkm::VecTraits<ValueType>::ComponentType>>&&
    componentVectors)
{
  return ArrayHandleSOA<ValueType>(std::move(componentVectors));
}

// This only works if all the templated arguments are of type std::vector<ComponentType>.
template <typename ComponentType, typename... RemainingVectors>
VTKM_CONT
  ArrayHandleSOA<vtkm::Vec<ComponentType, vtkm::IdComponent(sizeof...(RemainingVectors) + 1)>>
  make_ArrayHandleSOA(vtkm::CopyFlag copy,
                      const std::vector<ComponentType>& vector0,
                      const RemainingVectors&... componentVectors)
{
  return ArrayHandleSOA<
    vtkm::Vec<ComponentType, vtkm::IdComponent(sizeof...(RemainingVectors) + 1)>>(
    vector0, componentVectors..., copy);
}

template <typename ComponentType, typename... RemainingVectors>
VTKM_CONT
  ArrayHandleSOA<vtkm::Vec<ComponentType, vtkm::IdComponent(sizeof...(RemainingVectors) + 1)>>
  make_ArrayHandleSOA(const std::vector<ComponentType>& vector0,
                      const RemainingVectors&... componentVectors)
{
  return ArrayHandleSOA<
    vtkm::Vec<ComponentType, vtkm::IdComponent(sizeof...(RemainingVectors) + 1)>>(
    vector0, componentVectors...);
}

template <typename ValueType>
VTKM_CONT ArrayHandleSOA<ValueType> make_ArrayHandleSOA(
  std::initializer_list<const typename vtkm::VecTraits<ValueType>::ComponentType*>&&
    componentVectors,
  vtkm::Id length,
  vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  return ArrayHandleSOA<ValueType>(std::move(componentVectors), length, copy);
}

// This only works if all the templated arguments are of type std::vector<ComponentType>.
template <typename ComponentType, typename... RemainingArrays>
VTKM_CONT
  ArrayHandleSOA<vtkm::Vec<ComponentType, vtkm::IdComponent(sizeof...(RemainingArrays) + 1)>>
  make_ArrayHandleSOA(vtkm::Id length,
                      vtkm::CopyFlag copy,
                      const ComponentType* array0,
                      const RemainingArrays*... componentArrays)
{
  return ArrayHandleSOA<
    vtkm::Vec<ComponentType, vtkm::IdComponent(sizeof...(RemainingArrays) + 1)>>(
    length, copy, array0, componentArrays...);
}

template <typename ComponentType, typename... RemainingArrays>
VTKM_CONT
  ArrayHandleSOA<vtkm::Vec<ComponentType, vtkm::IdComponent(sizeof...(RemainingArrays) + 1)>>
  make_ArrayHandleSOA(vtkm::Id length,
                      const ComponentType* array0,
                      const RemainingArrays*... componentArrays)
{
  return ArrayHandleSOA<
    vtkm::Vec<ComponentType, vtkm::IdComponent(sizeof...(RemainingArrays) + 1)>>(
    length, array0, componentArrays...);
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

template <typename ValueType>
struct SerializableTypeString<vtkm::cont::ArrayHandleSOA<ValueType>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_SOA<" + SerializableTypeString<ValueType>::Get() + ">";
    return name;
  }
};

template <typename ValueType>
struct SerializableTypeString<vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagSOA>>
  : SerializableTypeString<vtkm::cont::ArrayHandleSOA<ValueType>>
{
};
}
} // namespace vtkm::cont

namespace mangled_diy_namespace
{

template <typename ValueType>
struct Serialization<vtkm::cont::ArrayHandleSOA<ValueType>>
{
  using BaseType = vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagSOA>;
  using Traits = vtkm::cont::internal::ArrayHandleSOATraits<ValueType>;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = Traits::NUM_COMPONENTS;

  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      vtkmdiy::save(bb, obj.GetStorage().GetArray(componentIndex));
    }
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      typename Traits::BaseArrayType componentArray;
      vtkmdiy::load(bb, componentArray);
      obj.GetStorage().SetArray(componentIndex, componentArray);
    }
  }
};

template <typename ValueType>
struct Serialization<vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagSOA>>
  : Serialization<vtkm::cont::ArrayHandleSOA<ValueType>>
{
};

} // namespace mangled_diy_namespace

//=============================================================================
// Precompiled instances

#ifndef vtkm_cont_ArrayHandleSOA_cxx

namespace vtkm
{
namespace cont
{

#define VTKM_ARRAYHANDLE_SOA_EXPORT(Type)                                                          \
  extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagSOA>;                \
  extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Vec<Type, 2>, StorageTagSOA>;  \
  extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Vec<Type, 3>, StorageTagSOA>;  \
  extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Vec<Type, 4>, StorageTagSOA>;

VTKM_ARRAYHANDLE_SOA_EXPORT(char)
VTKM_ARRAYHANDLE_SOA_EXPORT(vtkm::Int8)
VTKM_ARRAYHANDLE_SOA_EXPORT(vtkm::UInt8)
VTKM_ARRAYHANDLE_SOA_EXPORT(vtkm::Int16)
VTKM_ARRAYHANDLE_SOA_EXPORT(vtkm::UInt16)
VTKM_ARRAYHANDLE_SOA_EXPORT(vtkm::Int32)
VTKM_ARRAYHANDLE_SOA_EXPORT(vtkm::UInt32)
VTKM_ARRAYHANDLE_SOA_EXPORT(vtkm::Int64)
VTKM_ARRAYHANDLE_SOA_EXPORT(vtkm::UInt64)
VTKM_ARRAYHANDLE_SOA_EXPORT(vtkm::Float32)
VTKM_ARRAYHANDLE_SOA_EXPORT(vtkm::Float64)

#undef VTKM_ARRAYHANDLE_SOA_EXPORT
}
} // namespace vtkm::cont

#endif // !vtkm_cont_ArrayHandleSOA_cxx

#endif //vtk_m_cont_ArrayHandleSOA_h
