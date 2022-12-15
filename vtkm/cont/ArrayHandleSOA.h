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

#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/Math.h>
#include <vtkm/VecTraits.h>

#include <vtkm/internal/ArrayPortalBasic.h>
#include <vtkm/internal/ArrayPortalHelpers.h>

#include <vtkmstd/integer_sequence.h>

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
template <typename ValueType_, typename ComponentPortalType>
class ArrayPortalSOA
{
public:
  using ValueType = ValueType_;

private:
  using ComponentType = typename ComponentPortalType::ValueType;

  VTKM_STATIC_ASSERT(vtkm::HasVecTraits<ValueType>::value);
  using VTraits = vtkm::VecTraits<ValueType>;
  VTKM_STATIC_ASSERT((std::is_same<typename VTraits::ComponentType, ComponentType>::value));
  static constexpr vtkm::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

  ComponentPortalType Portals[NUM_COMPONENTS];
  vtkm::Id NumberOfValues;

public:
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT explicit ArrayPortalSOA(vtkm::Id numValues = 0)
    : NumberOfValues(numValues)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT void SetPortal(vtkm::IdComponent index, const ComponentPortalType& portal)
  {
    this->Portals[index] = portal;
  }

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  template <typename SPT = ComponentPortalType,
            typename Supported = typename vtkm::internal::PortalSupportsGets<SPT>::type,
            typename = typename std::enable_if<Supported::value>::type>
  VTKM_EXEC_CONT ValueType Get(vtkm::Id valueIndex) const
  {
    return this->Get(valueIndex, vtkmstd::make_index_sequence<NUM_COMPONENTS>());
  }

  template <typename SPT = ComponentPortalType,
            typename Supported = typename vtkm::internal::PortalSupportsSets<SPT>::type,
            typename = typename std::enable_if<Supported::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id valueIndex, const ValueType& value) const
  {
    this->Set(valueIndex, value, vtkmstd::make_index_sequence<NUM_COMPONENTS>());
  }

private:
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <std::size_t I>
  VTKM_EXEC_CONT ComponentType GetComponent(vtkm::Id valueIndex) const
  {
    return this->Portals[I].Get(valueIndex);
  }

  template <std::size_t... I>
  VTKM_EXEC_CONT ValueType Get(vtkm::Id valueIndex, vtkmstd::index_sequence<I...>) const
  {
    return ValueType{ this->GetComponent<I>(valueIndex)... };
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
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
                          vtkmstd::index_sequence<I...>) const
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

template <typename ComponentType, vtkm::IdComponent NUM_COMPONENTS>
class VTKM_ALWAYS_EXPORT
  Storage<vtkm::Vec<ComponentType, NUM_COMPONENTS>, vtkm::cont::StorageTagSOA>
{
  using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;

public:
  using ReadPortalType =
    vtkm::internal::ArrayPortalSOA<ValueType, vtkm::internal::ArrayPortalBasicRead<ComponentType>>;
  using WritePortalType =
    vtkm::internal::ArrayPortalSOA<ValueType, vtkm::internal::ArrayPortalBasicWrite<ComponentType>>;

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers()
  {
    return std::vector<vtkm::cont::internal::Buffer>(static_cast<std::size_t>(NUM_COMPONENTS));
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token)
  {
    vtkm::BufferSizeType numBytes =
      vtkm::internal::NumberOfValuesToNumberOfBytes<ComponentType>(numValues);
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      buffers[componentIndex].SetNumberOfBytes(numBytes, preserve, token);
    }
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    // Assume all buffers are the same size.
    return static_cast<vtkm::Id>(buffers[0].GetNumberOfBytes()) /
      static_cast<vtkm::Id>(sizeof(ComponentType));
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>& buffers,
                             const ValueType& fillValue,
                             vtkm::Id startIndex,
                             vtkm::Id endIndex,
                             vtkm::cont::Token& token)
  {
    constexpr vtkm::BufferSizeType sourceSize =
      static_cast<vtkm::BufferSizeType>(sizeof(ComponentType));
    vtkm::BufferSizeType startByte = startIndex * sourceSize;
    vtkm::BufferSizeType endByte = endIndex * sourceSize;
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      ComponentType source = fillValue[componentIndex];
      buffers[componentIndex].Fill(&source, sourceSize, startByte, endByte, token);
    }
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    vtkm::Id numValues = GetNumberOfValues(buffers);
    ReadPortalType portal(numValues);
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      VTKM_ASSERT(buffers[0].GetNumberOfBytes() == buffers[componentIndex].GetNumberOfBytes());
      portal.SetPortal(componentIndex,
                       vtkm::internal::ArrayPortalBasicRead<ComponentType>(
                         reinterpret_cast<const ComponentType*>(
                           buffers[componentIndex].ReadPointerDevice(device, token)),
                         numValues));
    }
    return portal;
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    vtkm::Id numValues = GetNumberOfValues(buffers);
    WritePortalType portal(numValues);
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      VTKM_ASSERT(buffers[0].GetNumberOfBytes() == buffers[componentIndex].GetNumberOfBytes());
      portal.SetPortal(componentIndex,
                       vtkm::internal::ArrayPortalBasicWrite<ComponentType>(
                         reinterpret_cast<ComponentType*>(
                           buffers[componentIndex].WritePointerDevice(device, token)),
                         numValues));
    }
    return portal;
  }
};

} // namespace internal

/// \brief An `ArrayHandle` that for Vecs stores each component in a separate physical array.
///
/// `ArrayHandleSOA` behaves like a regular `ArrayHandle` (with a basic storage) except that
/// if you specify a `ValueType` of a `Vec` or a `Vec-like`, it will actually store each
/// component in a separate physical array. When data are retrieved from the array, they are
/// reconstructed into `Vec` objects as expected.
///
/// The intention of this array type is to help cover the most common ways data is lain out in
/// memory. Typically, arrays of data are either an "array of structures" like the basic storage
/// where you have a single array of structures (like `Vec`) or a "structure of arrays" where
/// you have an array of a basic type (like `float`) for each component of the data being
/// represented. The `ArrayHandleSOA` makes it easy to cover this second case without creating
/// special types.
///
/// `ArrayHandleSOA` can be constructed from a collection of `ArrayHandle` with basic storage.
/// This allows you to construct `Vec` arrays from components without deep copies.
///
template <typename T>
class ArrayHandleSOA : public ArrayHandle<T, vtkm::cont::StorageTagSOA>
{
  using ComponentType = typename vtkm::VecTraits<T>::ComponentType;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = vtkm::VecTraits<T>::NUM_COMPONENTS;

  using StorageType = vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagSOA>;

  using ComponentArrayType = vtkm::cont::ArrayHandle<ComponentType, vtkm::cont::StorageTagBasic>;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleSOA,
                             (ArrayHandleSOA<T>),
                             (ArrayHandle<T, vtkm::cont::StorageTagSOA>));

  ArrayHandleSOA(std::initializer_list<vtkm::cont::internal::Buffer>&& componentBuffers)
    : Superclass(std::move(componentBuffers))
  {
  }

  ArrayHandleSOA(const std::array<ComponentArrayType, NUM_COMPONENTS>& componentArrays)
  {
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      this->SetArray(componentIndex, componentArrays[componentIndex]);
    }
  }

  ArrayHandleSOA(const std::vector<ComponentArrayType>& componentArrays)
  {
    VTKM_ASSERT(componentArrays.size() == NUM_COMPONENTS);
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      this->SetArray(componentIndex, componentArrays[componentIndex]);
    }
  }

  ArrayHandleSOA(std::initializer_list<ComponentArrayType>&& componentArrays)
  {
    VTKM_ASSERT(componentArrays.size() == NUM_COMPONENTS);
    vtkm::IdComponent componentIndex = 0;
    for (auto&& array : componentArrays)
    {
      this->SetArray(componentIndex, array);
      ++componentIndex;
    }
  }

  ArrayHandleSOA(std::initializer_list<std::vector<ComponentType>>&& componentVectors)
  {
    VTKM_ASSERT(componentVectors.size() == NUM_COMPONENTS);
    vtkm::IdComponent componentIndex = 0;
    for (auto&& vector : componentVectors)
    {
      // Note, std::vectors that come from std::initializer_list must be copied because the scope
      // of the objects in the initializer list disappears.
      this->SetArray(componentIndex, vtkm::cont::make_ArrayHandle(vector, vtkm::CopyFlag::On));
      ++componentIndex;
    }
  }

  // This only works if all the templated arguments are of type std::vector<ComponentType>.
  template <typename Allocator, typename... RemainingVectors>
  ArrayHandleSOA(vtkm::CopyFlag copy,
                 const std::vector<ComponentType, Allocator>& vector0,
                 RemainingVectors&&... componentVectors)
    : Superclass(std::vector<vtkm::cont::internal::Buffer>{
        vtkm::cont::make_ArrayHandle(vector0, copy).GetBuffers()[0],
        vtkm::cont::make_ArrayHandle(std::forward<RemainingVectors>(componentVectors), copy)
          .GetBuffers()[0]... })
  {
    VTKM_STATIC_ASSERT(sizeof...(RemainingVectors) + 1 == NUM_COMPONENTS);
  }

  // This only works if all the templated arguments are of type std::vector<ComponentType>.
  template <typename... RemainingVectors>
  ArrayHandleSOA(vtkm::CopyFlag copy,
                 std::vector<ComponentType>&& vector0,
                 RemainingVectors&&... componentVectors)
    : Superclass(std::vector<vtkm::cont::internal::Buffer>{
        vtkm::cont::make_ArrayHandle(std::move(vector0), copy),
        vtkm::cont::make_ArrayHandle(std::forward<RemainingVectors>(componentVectors), copy)
          .GetBuffers()[0]... })
  {
    VTKM_STATIC_ASSERT(sizeof...(RemainingVectors) + 1 == NUM_COMPONENTS);
  }

  ArrayHandleSOA(std::initializer_list<const ComponentType*> componentArrays,
                 vtkm::Id length,
                 vtkm::CopyFlag copy)
  {
    VTKM_ASSERT(componentArrays.size() == NUM_COMPONENTS);
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
    : Superclass(std::vector<vtkm::cont::internal::Buffer>{
        vtkm::cont::make_ArrayHandle(array0, length, copy).GetBuffers()[0],
        vtkm::cont::make_ArrayHandle(componentArrays, length, copy).GetBuffers()[0]... })
  {
    VTKM_STATIC_ASSERT(sizeof...(RemainingArrays) + 1 == NUM_COMPONENTS);
  }

  VTKM_CONT vtkm::cont::ArrayHandleBasic<ComponentType> GetArray(vtkm::IdComponent index) const
  {
    return ComponentArrayType({ this->GetBuffers()[index] });
  }

  VTKM_CONT void SetArray(vtkm::IdComponent index, const ComponentArrayType& array)
  {
    this->SetBuffer(index, array.GetBuffers()[0]);
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
                      RemainingVectors&&... componentVectors)
{
  // Convert std::vector to ArrayHandle first so that it correctly handles a mix of rvalue args.
  return { vtkm::cont::make_ArrayHandle(vector0, copy),
           vtkm::cont::make_ArrayHandle(std::forward<RemainingVectors>(componentVectors),
                                        copy)... };
}

// This only works if all the templated arguments are of type std::vector<ComponentType>.
template <typename ComponentType, typename... RemainingVectors>
VTKM_CONT
  ArrayHandleSOA<vtkm::Vec<ComponentType, vtkm::IdComponent(sizeof...(RemainingVectors) + 1)>>
  make_ArrayHandleSOA(vtkm::CopyFlag copy,
                      std::vector<ComponentType>&& vector0,
                      RemainingVectors&&... componentVectors)
{
  // Convert std::vector to ArrayHandle first so that it correctly handles a mix of rvalue args.
  return ArrayHandleSOA<
    vtkm::Vec<ComponentType, vtkm::IdComponent(sizeof...(RemainingVectors) + 1)>>(
    vtkm::cont::make_ArrayHandle(std::move(vector0), copy),
    vtkm::cont::make_ArrayHandle(std::forward<RemainingVectors>(componentVectors), copy)...);
}

// This only works if all the templated arguments are rvalues of std::vector<ComponentType>.
template <typename ComponentType, typename... RemainingVectors>
VTKM_CONT
  ArrayHandleSOA<vtkm::Vec<ComponentType, vtkm::IdComponent(sizeof...(RemainingVectors) + 1)>>
  make_ArrayHandleSOAMove(std::vector<ComponentType>&& vector0,
                          RemainingVectors&&... componentVectors)
{
  return { vtkm::cont::make_ArrayHandleMove(std::move(vector0)),
           vtkm::cont::make_ArrayHandleMove(std::forward<RemainingVectors>(componentVectors))... };
}

template <typename ValueType>
VTKM_CONT ArrayHandleSOA<ValueType> make_ArrayHandleSOA(
  std::initializer_list<const typename vtkm::VecTraits<ValueType>::ComponentType*>&&
    componentVectors,
  vtkm::Id length,
  vtkm::CopyFlag copy)
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

namespace internal
{

template <>
struct ArrayExtractComponentImpl<vtkm::cont::StorageTagSOA>
{
  template <typename T>
  auto operator()(const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagSOA>& src,
                  vtkm::IdComponent componentIndex,
                  vtkm::CopyFlag allowCopy) const
    -> decltype(
      ArrayExtractComponentImpl<vtkm::cont::StorageTagBasic>{}(vtkm::cont::ArrayHandleBasic<T>{},
                                                               componentIndex,
                                                               allowCopy))
  {
    using FirstLevelComponentType = typename vtkm::VecTraits<T>::ComponentType;
    vtkm::cont::ArrayHandleSOA<T> array(src);
    constexpr vtkm::IdComponent NUM_SUB_COMPONENTS =
      vtkm::VecFlat<FirstLevelComponentType>::NUM_COMPONENTS;
    return ArrayExtractComponentImpl<vtkm::cont::StorageTagBasic>{}(
      array.GetArray(componentIndex / NUM_SUB_COMPONENTS),
      componentIndex % NUM_SUB_COMPONENTS,
      allowCopy);
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
  static constexpr vtkm::IdComponent NUM_COMPONENTS = vtkm::VecTraits<ValueType>::NUM_COMPONENTS;

  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      vtkmdiy::save(bb, obj.GetBuffers()[componentIndex]);
    }
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    std::vector<vtkm::cont::internal::Buffer> buffers(NUM_COMPONENTS);
    for (std::size_t componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      vtkmdiy::load(bb, buffers[componentIndex]);
    }
    obj = BaseType(buffers);
  }
};

template <typename ValueType>
struct Serialization<vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagSOA>>
  : Serialization<vtkm::cont::ArrayHandleSOA<ValueType>>
{
};

} // namespace mangled_diy_namespace
// @endcond SERIALIZATION

//=============================================================================
// Precompiled instances

#ifndef vtkm_cont_ArrayHandleSOA_cxx

namespace vtkm
{
namespace cont
{

#define VTKM_ARRAYHANDLE_SOA_EXPORT(Type)                                                         \
  extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Vec<Type, 2>, StorageTagSOA>; \
  extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Vec<Type, 3>, StorageTagSOA>; \
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
