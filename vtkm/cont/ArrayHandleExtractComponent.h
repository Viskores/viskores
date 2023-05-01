//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleExtractComponent_h
#define vtk_m_cont_ArrayHandleExtractComponent_h

#include <vtkm/StaticAssert.h>
#include <vtkm/VecTraits.h>
#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace internal
{

template <typename PortalType>
class VTKM_ALWAYS_EXPORT ArrayPortalExtractComponent
{
  using Writable = vtkm::internal::PortalSupportsSets<PortalType>;

public:
  using VectorType = typename PortalType::ValueType;
  using Traits = vtkm::VecTraits<VectorType>;
  using ValueType = typename Traits::ComponentType;

  VTKM_EXEC_CONT
  ArrayPortalExtractComponent()
    : Portal()
    , Component(0)
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalExtractComponent(const PortalType& portal, vtkm::IdComponent component)
    : Portal(portal)
    , Component(component)
  {
  }

  ArrayPortalExtractComponent(const ArrayPortalExtractComponent&) = default;
  ArrayPortalExtractComponent(ArrayPortalExtractComponent&&) = default;
  ArrayPortalExtractComponent& operator=(const ArrayPortalExtractComponent&) = default;
  ArrayPortalExtractComponent& operator=(ArrayPortalExtractComponent&&) = default;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    return Traits::GetComponent(this->Portal.Get(index), this->Component);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    VectorType vec = this->Portal.Get(index);
    Traits::SetComponent(vec, this->Component, value);
    this->Portal.Set(index, vec);
  }

  VTKM_EXEC_CONT
  const PortalType& GetPortal() const { return this->Portal; }

private:
  PortalType Portal;
  vtkm::IdComponent Component;
}; // class ArrayPortalExtractComponent

} // namespace internal

namespace cont
{

template <typename ArrayHandleType>
class StorageTagExtractComponent
{
};

namespace internal
{

template <typename ArrayHandleType>
class Storage<typename vtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
              StorageTagExtractComponent<ArrayHandleType>>
{
  using SourceValueType = typename ArrayHandleType::ValueType;
  using ValueType = typename vtkm::VecTraits<SourceValueType>::ComponentType;
  using SourceStorage = typename ArrayHandleType::StorageType;

public:
  VTKM_CONT static vtkm::IdComponent ComponentIndex(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<vtkm::IdComponent>();
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> SourceBuffers(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return std::vector<vtkm::cont::internal::Buffer>(buffers.begin() + 1, buffers.end());
  }

  using ReadPortalType =
    vtkm::internal::ArrayPortalExtractComponent<typename SourceStorage::ReadPortalType>;
  using WritePortalType =
    vtkm::internal::ArrayPortalExtractComponent<typename SourceStorage::WritePortalType>;

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return SourceStorage::GetNumberOfValues(SourceBuffers(buffers));
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>&,
                             const ValueType&,
                             vtkm::Id,
                             vtkm::Id,
                             vtkm::cont::Token&)
  {
    throw vtkm::cont::ErrorBadType("Fill not supported for ArrayHandleExtractComponent.");
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token)
  {
    SourceStorage::ResizeBuffers(numValues, SourceBuffers(buffers), preserve, token);
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return ReadPortalType(SourceStorage::CreateReadPortal(SourceBuffers(buffers), device, token),
                          ComponentIndex(buffers));
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return WritePortalType(SourceStorage::CreateWritePortal(SourceBuffers(buffers), device, token),
                           ComponentIndex(buffers));
  }

  VTKM_CONT static auto CreateBuffers(vtkm::IdComponent componentIndex = 0,
                                      const ArrayHandleType& array = ArrayHandleType{})
    -> decltype(vtkm::cont::internal::CreateBuffers())
  {
    return vtkm::cont::internal::CreateBuffers(componentIndex, array);
  }
}; // class Storage

}
}
} // namespace vtkm::cont::internal

namespace vtkm
{
namespace cont
{

/// \brief A fancy ArrayHandle that turns a vector array into a scalar array by
/// slicing out a single component of each vector.
///
/// ArrayHandleExtractComponent is a specialization of ArrayHandle. It takes an
/// input ArrayHandle with a vtkm::Vec ValueType and a component index
/// and uses this information to expose a scalar array consisting of the
/// specified component across all vectors in the input ArrayHandle. So for a
/// given index i, ArrayHandleExtractComponent looks up the i-th vtkm::Vec in
/// the index array and reads or writes to the specified component, leave all
/// other components unmodified. This is done on the fly rather than creating a
/// copy of the array.
template <typename ArrayHandleType>
class ArrayHandleExtractComponent
  : public vtkm::cont::ArrayHandle<
      typename vtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
      StorageTagExtractComponent<ArrayHandleType>>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleExtractComponent,
    (ArrayHandleExtractComponent<ArrayHandleType>),
    (vtkm::cont::ArrayHandle<
      typename vtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
      StorageTagExtractComponent<ArrayHandleType>>));

  VTKM_CONT
  ArrayHandleExtractComponent(const ArrayHandleType& array, vtkm::IdComponent component)
    : Superclass(StorageType::CreateBuffers(component, array))
  {
  }

  VTKM_CONT vtkm::IdComponent GetComponent() const
  {
    return StorageType::ComponentIndex(this->GetBuffers());
  }

  VTKM_CONT ArrayHandleType GetArray() const
  {
    using BaseArray = vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                              typename ArrayHandleType::StorageTag>;
    return ArrayHandleType(BaseArray(StorageType::SourceBuffers(this->GetBuffers())));
  }
};

/// make_ArrayHandleExtractComponent is convenience function to generate an
/// ArrayHandleExtractComponent.
template <typename ArrayHandleType>
VTKM_CONT ArrayHandleExtractComponent<ArrayHandleType> make_ArrayHandleExtractComponent(
  const ArrayHandleType& array,
  vtkm::IdComponent component)
{
  return ArrayHandleExtractComponent<ArrayHandleType>(array, component);
}

namespace internal
{

template <typename ArrayHandleType>
struct ArrayExtractComponentImpl<vtkm::cont::StorageTagExtractComponent<ArrayHandleType>>
{
  auto operator()(const vtkm::cont::ArrayHandleExtractComponent<ArrayHandleType>& src,
                  vtkm::IdComponent componentIndex,
                  vtkm::CopyFlag allowCopy) const
    -> decltype(ArrayExtractComponentImpl<typename ArrayHandleType::StorageTag>{}(
      std::declval<ArrayHandleType>(),
      componentIndex,
      allowCopy))
  {
    using ValueType = typename ArrayHandleType::ValueType;
    using ComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;
    using FlatComponent = vtkm::VecFlat<ComponentType>;
    constexpr vtkm::IdComponent FLAT_SUB_COMPONENTS = FlatComponent::NUM_COMPONENTS;
    return ArrayExtractComponentImpl<typename ArrayHandleType::StorageTag>{}(
      src.GetArray(), (src.GetComponent() * FLAT_SUB_COMPONENTS) + componentIndex, allowCopy);
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

template <typename AH>
struct SerializableTypeString<vtkm::cont::ArrayHandleExtractComponent<AH>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_ExtractComponent<" + SerializableTypeString<AH>::Get() + ">";
    return name;
  }
};

template <typename AH>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<typename vtkm::VecTraits<typename AH::ValueType>::ComponentType,
                          vtkm::cont::StorageTagExtractComponent<AH>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleExtractComponent<AH>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH>
struct Serialization<vtkm::cont::ArrayHandleExtractComponent<AH>>
{
private:
  using Type = vtkm::cont::ArrayHandleExtractComponent<AH>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, Type(obj).GetComponent());
    vtkmdiy::save(bb, Type(obj).GetArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    vtkm::IdComponent component = 0;
    AH array;
    vtkmdiy::load(bb, component);
    vtkmdiy::load(bb, array);

    obj = vtkm::cont::make_ArrayHandleExtractComponent(array, component);
  }
};

template <typename AH>
struct Serialization<
  vtkm::cont::ArrayHandle<typename vtkm::VecTraits<typename AH::ValueType>::ComponentType,
                          vtkm::cont::StorageTagExtractComponent<AH>>>
  : Serialization<vtkm::cont::ArrayHandleExtractComponent<AH>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif // vtk_m_cont_ArrayHandleExtractComponent_h
