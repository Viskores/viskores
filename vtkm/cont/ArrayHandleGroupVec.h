//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleGroupVec_h
#define vtk_m_cont_ArrayHandleGroupVec_h

#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/ErrorBadValue.h>

namespace vtkm
{
namespace internal
{

template <typename PortalType, vtkm::IdComponent N_COMPONENTS>
class VTKM_ALWAYS_EXPORT ArrayPortalGroupVec
{
  using Writable = vtkm::internal::PortalSupportsSets<PortalType>;

public:
  static constexpr vtkm::IdComponent NUM_COMPONENTS = N_COMPONENTS;
  using ComponentsPortalType = PortalType;

  using ComponentType = typename std::remove_const<typename ComponentsPortalType::ValueType>::type;
  using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalGroupVec()
    : ComponentsPortal()
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalGroupVec(const ComponentsPortalType& componentsPortal)
    : ComponentsPortal(componentsPortal)
  {
  }

  /// Copy constructor for any other ArrayPortalConcatenate with a portal type
  /// that can be copied to this portal type. This allows us to do any type
  /// casting that the portals do (like the non-const to const cast).
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OtherComponentsPortalType>
  VTKM_EXEC_CONT ArrayPortalGroupVec(
    const ArrayPortalGroupVec<OtherComponentsPortalType, NUM_COMPONENTS>& src)
    : ComponentsPortal(src.GetPortal())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->ComponentsPortal.GetNumberOfValues() / NUM_COMPONENTS;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    ValueType result;
    vtkm::Id componentsIndex = index * NUM_COMPONENTS;
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
    {
      result[componentIndex] = this->ComponentsPortal.Get(componentsIndex);
      componentsIndex++;
    }
    return result;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    vtkm::Id componentsIndex = index * NUM_COMPONENTS;
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
    {
      this->ComponentsPortal.Set(componentsIndex, value[componentIndex]);
      componentsIndex++;
    }
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const ComponentsPortalType& GetPortal() const { return this->ComponentsPortal; }

private:
  ComponentsPortalType ComponentsPortal;
};
}
} // namespace vtkm::internal

namespace vtkm
{
namespace cont
{

template <typename ComponentsStorageTag, vtkm::IdComponent NUM_COMPONENTS>
struct VTKM_ALWAYS_EXPORT StorageTagGroupVec
{
};

namespace internal
{

template <typename ComponentType, vtkm::IdComponent NUM_COMPONENTS, typename ComponentsStorageTag>
class Storage<vtkm::Vec<ComponentType, NUM_COMPONENTS>,
              vtkm::cont::StorageTagGroupVec<ComponentsStorageTag, NUM_COMPONENTS>>
{
  using ComponentsStorage = vtkm::cont::internal::Storage<ComponentType, ComponentsStorageTag>;
  using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;

public:
  using ReadPortalType =
    vtkm::internal::ArrayPortalGroupVec<typename ComponentsStorage::ReadPortalType, NUM_COMPONENTS>;
  using WritePortalType =
    vtkm::internal::ArrayPortalGroupVec<typename ComponentsStorage::WritePortalType,
                                        NUM_COMPONENTS>;

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers()
  {
    return ComponentsStorage::CreateBuffers();
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token)
  {
    ComponentsStorage::ResizeBuffers(NUM_COMPONENTS * numValues, buffers, preserve, token);
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    vtkm::Id componentsSize = ComponentsStorage::GetNumberOfValues(buffers);
    return componentsSize / NUM_COMPONENTS;
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>&,
                             const ValueType&,
                             vtkm::Id,
                             vtkm::Id,
                             vtkm::cont::Token&)
  {
    throw vtkm::cont::ErrorBadType("Fill not supported for ArrayHandleGroupVec.");
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    if ((ComponentsStorage::GetNumberOfValues(buffers) % NUM_COMPONENTS) != 0)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                 "ArrayHandleGroupVec's components array does not divide evenly into Vecs.");
    }
    return ReadPortalType(ComponentsStorage::CreateReadPortal(buffers, device, token));
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    if ((ComponentsStorage::GetNumberOfValues(buffers) % NUM_COMPONENTS) != 0)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                 "ArrayHandleGroupVec's components array does not divide evenly into Vecs.");
    }
    return WritePortalType(ComponentsStorage::CreateWritePortal(buffers, device, token));
  }
};

} // namespace internal

/// \brief Fancy array handle that groups values into vectors.
///
/// It is sometimes the case that an array is stored such that consecutive
/// entries are meant to form a group. This fancy array handle takes an array
/// of values and a size of groups and then groups the consecutive values
/// stored in a \c Vec.
///
/// For example, if you have an array handle with the six values 0,1,2,3,4,5
/// and give it to a \c ArrayHandleGroupVec with the number of components set
/// to 3, you get an array that looks like it contains two values of \c Vec
/// values of size 3 with the data [0,1,2], [3,4,5].
///
/// The array of components should have a number of values that divides evenly
/// with the size of the Vec. If the components array does not divide evenly
/// into `Vec`s, then a warning will be logged and the extra component values
/// will be ignored.
///
template <typename ComponentsArrayHandleType, vtkm::IdComponent NUM_COMPONENTS>
class ArrayHandleGroupVec
  : public vtkm::cont::ArrayHandle<
      vtkm::Vec<typename ComponentsArrayHandleType::ValueType, NUM_COMPONENTS>,
      vtkm::cont::StorageTagGroupVec<typename ComponentsArrayHandleType::StorageTag,
                                     NUM_COMPONENTS>>
{
  VTKM_IS_ARRAY_HANDLE(ComponentsArrayHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleGroupVec,
    (ArrayHandleGroupVec<ComponentsArrayHandleType, NUM_COMPONENTS>),
    (vtkm::cont::ArrayHandle<
      vtkm::Vec<typename ComponentsArrayHandleType::ValueType, NUM_COMPONENTS>,
      vtkm::cont::StorageTagGroupVec<typename ComponentsArrayHandleType::StorageTag,
                                     NUM_COMPONENTS>>));

  using ComponentType = typename ComponentsArrayHandleType::ValueType;

  VTKM_CONT
  ArrayHandleGroupVec(const ComponentsArrayHandleType& componentsArray)
    : Superclass(componentsArray.GetBuffers())
  {
  }

  VTKM_CONT ComponentsArrayHandleType GetComponentsArray() const
  {
    return ComponentsArrayHandleType(this->GetBuffers());
  }
};

/// \c make_ArrayHandleGroupVec is convenience function to generate an
/// ArrayHandleGroupVec. It takes in an ArrayHandle and the number of components
/// (as a specified template parameter), and returns an array handle with
/// consecutive entries grouped in a Vec.
///
template <vtkm::IdComponent NUM_COMPONENTS, typename ArrayHandleType>
VTKM_CONT vtkm::cont::ArrayHandleGroupVec<ArrayHandleType, NUM_COMPONENTS> make_ArrayHandleGroupVec(
  const ArrayHandleType& array)
{
  return vtkm::cont::ArrayHandleGroupVec<ArrayHandleType, NUM_COMPONENTS>(array);
}

//--------------------------------------------------------------------------------
// Specialization of ArrayExtractComponent
namespace internal
{

// Superclass will inherit the ArrayExtractComponentImplInefficient property if
// the sub-storage is inefficient (thus making everything inefficient).
template <typename ComponentsStorageTag, vtkm::IdComponent NUM_COMPONENTS>
struct ArrayExtractComponentImpl<
  vtkm::cont::StorageTagGroupVec<ComponentsStorageTag, NUM_COMPONENTS>>
  : vtkm::cont::internal::ArrayExtractComponentImpl<ComponentsStorageTag>
{
  template <typename T>
  vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType> operator()(
    const vtkm::cont::ArrayHandle<
      vtkm::Vec<T, NUM_COMPONENTS>,
      vtkm::cont::StorageTagGroupVec<ComponentsStorageTag, NUM_COMPONENTS>>& src,
    vtkm::IdComponent componentIndex,
    vtkm::CopyFlag allowCopy) const
  {
    vtkm::cont::ArrayHandleGroupVec<vtkm::cont::ArrayHandle<T, ComponentsStorageTag>,
                                    NUM_COMPONENTS>
      srcArray(src);
    constexpr vtkm::IdComponent NUM_SUB_COMPONENTS = vtkm::VecFlat<T>::NUM_COMPONENTS;
    vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType> dest =
      ArrayExtractComponentImpl<ComponentsStorageTag>{}(
        srcArray.GetComponentsArray(), componentIndex % NUM_SUB_COMPONENTS, allowCopy);

    // Adjust stride and offset to expectations of grouped values
    return vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType>(
      dest.GetBasicArray(),
      dest.GetNumberOfValues() / NUM_COMPONENTS,
      dest.GetStride() * NUM_COMPONENTS,
      dest.GetOffset() + (dest.GetStride() * (componentIndex / NUM_SUB_COMPONENTS)),
      dest.GetModulo(),
      dest.GetDivisor());
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

template <typename AH, vtkm::IdComponent NUM_COMPS>
struct SerializableTypeString<vtkm::cont::ArrayHandleGroupVec<AH, NUM_COMPS>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name =
      "AH_GroupVec<" + SerializableTypeString<AH>::Get() + "," + std::to_string(NUM_COMPS) + ">";
    return name;
  }
};

template <typename T, vtkm::IdComponent NUM_COMPS, typename ST>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<vtkm::Vec<T, NUM_COMPS>, vtkm::cont::StorageTagGroupVec<ST, NUM_COMPS>>>
  : SerializableTypeString<
      vtkm::cont::ArrayHandleGroupVec<vtkm::cont::ArrayHandle<T, ST>, NUM_COMPS>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH, vtkm::IdComponent NUM_COMPS>
struct Serialization<vtkm::cont::ArrayHandleGroupVec<AH, NUM_COMPS>>
{
private:
  using Type = vtkm::cont::ArrayHandleGroupVec<AH, NUM_COMPS>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, Type(obj).GetComponentsArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    vtkmdiy::load(bb, array);

    obj = vtkm::cont::make_ArrayHandleGroupVec<NUM_COMPS>(array);
  }
};

template <typename T, vtkm::IdComponent NUM_COMPS, typename ST>
struct Serialization<
  vtkm::cont::ArrayHandle<vtkm::Vec<T, NUM_COMPS>, vtkm::cont::StorageTagGroupVec<ST, NUM_COMPS>>>
  : Serialization<vtkm::cont::ArrayHandleGroupVec<vtkm::cont::ArrayHandle<T, ST>, NUM_COMPS>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleGroupVec_h
