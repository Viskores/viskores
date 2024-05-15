//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleRuntimeVec_h
#define vtk_m_cont_ArrayHandleRuntimeVec_h

#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/ErrorBadType.h>

#include <vtkm/Assert.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/VecFromPortal.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace internal
{

namespace detail
{

template <typename T>
struct UnrollVecImpl
{
  using type = vtkm::Vec<T, 1>;
};

template <typename T, vtkm::IdComponent N>
struct UnrollVecImpl<vtkm::Vec<T, N>>
{
  using subtype = typename UnrollVecImpl<T>::type;
  using type = vtkm::Vec<typename subtype::ComponentType, subtype::NUM_COMPONENTS * N>;
};

} // namespace detail

// A helper class that unrolls a nested `Vec` to a single layer `Vec`. This is similar
// to `vtkm::VecFlat`, except that this only flattens `vtkm::Vec<T,N>` objects, and not
// any other `Vec`-like objects. The reason is that a `vtkm::Vec<T,N>` is the same as N
// consecutive `T` objects whereas the same may not be said about other `Vec`-like objects.
template <typename T>
using UnrollVec = typename detail::UnrollVecImpl<T>::type;

template <typename ComponentsPortalType>
class VTKM_ALWAYS_EXPORT ArrayPortalRuntimeVec
{
public:
  using ComponentType = typename std::remove_const<typename ComponentsPortalType::ValueType>::type;
  using ValueType = vtkm::VecFromPortal<ComponentsPortalType>;

  ArrayPortalRuntimeVec() = default;

  VTKM_EXEC_CONT ArrayPortalRuntimeVec(const ComponentsPortalType& componentsPortal,
                                       vtkm::IdComponent numComponents)
    : ComponentsPortal(componentsPortal)
    , NumberOfComponents(numComponents)
  {
  }

  /// Copy constructor for any other ArrayPortalRuntimeVec with a portal type
  /// that can be copied to this portal type. This allows us to do any type
  /// casting that the portals do (like the non-const to const cast).
  template <typename OtherComponentsPortalType>
  VTKM_EXEC_CONT ArrayPortalRuntimeVec(const ArrayPortalRuntimeVec<OtherComponentsPortalType>& src)
    : ComponentsPortal(src.GetComponentsPortal())
    , NumberOfComponents(src.GetNumberOfComponents())
  {
  }

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const
  {
    return this->ComponentsPortal.GetNumberOfValues() / this->NumberOfComponents;
  }

  VTKM_EXEC_CONT ValueType Get(vtkm::Id index) const
  {
    return ValueType(
      this->ComponentsPortal, this->NumberOfComponents, index * this->NumberOfComponents);
  }

  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    if ((&value.GetPortal() == &this->ComponentsPortal) &&
        (value.GetOffset() == (index * this->NumberOfComponents)))
    {
      // The ValueType (VecFromPortal) operates on demand. Thus, if you set
      // something in the value, it has already been passed to the array.
    }
    else
    {
      // The value comes from somewhere else. Copy data in.
      this->Get(index) = value;
    }
  }

  VTKM_EXEC_CONT const ComponentsPortalType& GetComponentsPortal() const
  {
    return this->ComponentsPortal;
  }

  VTKM_EXEC_CONT vtkm::IdComponent GetNumberOfComponents() const
  {
    return this->NumberOfComponents;
  }

private:
  ComponentsPortalType ComponentsPortal;
  vtkm::IdComponent NumberOfComponents = 0;
};

}
} // namespace vtkm::internal

namespace vtkm
{
namespace cont
{

struct VTKM_ALWAYS_EXPORT StorageTagRuntimeVec
{
};

namespace internal
{

struct RuntimeVecMetaData
{
  vtkm::IdComponent NumberOfComponents;
};

template <typename ComponentsPortal>
class Storage<vtkm::VecFromPortal<ComponentsPortal>, vtkm::cont::StorageTagRuntimeVec>
{
  using ComponentType = typename ComponentsPortal::ValueType;
  using ComponentsStorage =
    vtkm::cont::internal::Storage<ComponentType, vtkm::cont::StorageTagBasic>;

  VTKM_STATIC_ASSERT_MSG(
    vtkm::VecTraits<ComponentType>::NUM_COMPONENTS == 1,
    "ArrayHandleRuntimeVec only supports scalars grouped into a single Vec. Nested Vecs can "
    "still be used with ArrayHandleRuntimeVec. The values are treated as flattened (like "
    "with VecFlat).");

  using ComponentsArray = vtkm::cont::ArrayHandle<ComponentType, StorageTagBasic>;

  VTKM_STATIC_ASSERT_MSG(
    (std::is_same<ComponentsPortal, typename ComponentsStorage::WritePortalType>::value),
    "Used invalid ComponentsPortal type with expected ComponentsStorageTag.");

  using Info = RuntimeVecMetaData;

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> ComponentsBuffers(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return std::vector<vtkm::cont::internal::Buffer>(buffers.begin() + 1, buffers.end());
  }

public:
  using ReadPortalType =
    vtkm::internal::ArrayPortalRuntimeVec<typename ComponentsStorage::ReadPortalType>;
  using WritePortalType =
    vtkm::internal::ArrayPortalRuntimeVec<typename ComponentsStorage::WritePortalType>;

  VTKM_CONT static vtkm::IdComponent GetNumberOfComponents(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<Info>().NumberOfComponents;
  }

  VTKM_CONT static vtkm::IdComponent GetNumberOfComponentsFlat(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    vtkm::IdComponent numComponents = GetNumberOfComponents(buffers);
    vtkm::IdComponent numSubComponents =
      ComponentsStorage::GetNumberOfComponentsFlat(ComponentsBuffers(buffers));
    return numComponents * numSubComponents;
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return ComponentsStorage::GetNumberOfValues(ComponentsBuffers(buffers)) /
      GetNumberOfComponents(buffers);
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token)
  {
    ComponentsStorage::ResizeBuffers(
      numValues * GetNumberOfComponents(buffers), ComponentsBuffers(buffers), preserve, token);
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>&,
                             const vtkm::VecFromPortal<ComponentsPortal>&,
                             vtkm::Id,
                             vtkm::Id,
                             vtkm::cont::Token&)
  {
    throw vtkm::cont::ErrorBadType("Fill not supported for ArrayHandleRuntimeVec.");
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return ReadPortalType(
      ComponentsStorage::CreateReadPortal(ComponentsBuffers(buffers), device, token),
      GetNumberOfComponents(buffers));
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return WritePortalType(
      ComponentsStorage::CreateWritePortal(ComponentsBuffers(buffers), device, token),
      GetNumberOfComponents(buffers));
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(
    vtkm::IdComponent numComponents = 1,
    const ComponentsArray& componentsArray = ComponentsArray{})
  {
    VTKM_LOG_IF_S(vtkm::cont::LogLevel::Warn,
                  (componentsArray.GetNumberOfValues() % numComponents) != 0,
                  "Array given to ArrayHandleRuntimeVec has size ("
                    << componentsArray.GetNumberOfValues()
                    << ") that is not divisible by the number of components selected ("
                    << numComponents << ").");
    Info info;
    info.NumberOfComponents = numComponents;
    return vtkm::cont::internal::CreateBuffers(info, componentsArray);
  }

  VTKM_CONT static ComponentsArray GetComponentsArray(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return ComponentsArray(ComponentsBuffers(buffers));
  }

  VTKM_CONT static void AsArrayHandleBasic(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::ArrayHandle<ComponentType, vtkm::cont::StorageTagBasic>& dest)
  {
    if (GetNumberOfComponents(buffers) != 1)
    {
      throw vtkm::cont::ErrorBadType(
        "Attempted to pull a scalar array from an ArrayHandleRuntime that does not hold scalars.");
    }
    dest = GetComponentsArray(buffers);
  }

  template <vtkm::IdComponent N>
  VTKM_CONT static void AsArrayHandleBasic(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::ArrayHandle<vtkm::Vec<ComponentType, N>, vtkm::cont::StorageTagBasic>& dest)
  {
    if (GetNumberOfComponents(buffers) != N)
    {
      throw vtkm::cont::ErrorBadType(
        "Attempted to pull an array of Vecs of the wrong size from an ArrayHandleRuntime.");
    }
    dest = vtkm::cont::ArrayHandle<vtkm::Vec<ComponentType, N>, vtkm::cont::StorageTagBasic>(
      ComponentsBuffers(buffers));
  }

  template <typename T, vtkm::IdComponent NInner, vtkm::IdComponent NOuter>
  VTKM_CONT static void AsArrayHandleBasic(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec<T, NInner>, NOuter>, vtkm::cont::StorageTagBasic>&
      dest)
  {
    // Flatten the Vec by one level and attempt to get the array handle for that.
    vtkm::cont::ArrayHandleBasic<vtkm::Vec<T, NInner * NOuter>> squashedArray;
    AsArrayHandleBasic(buffers, squashedArray);
    // Now unsquash the array by stealling the buffers and creating an array of the right type
    dest =
      vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec<T, NInner>, NOuter>, vtkm::cont::StorageTagBasic>(
        squashedArray.GetBuffers());
  }
};

} // namespace internal

/// @brief Fancy array handle for a basic array with runtime selected vec size.
///
/// It is sometimes the case that you need to create an array of `Vec`s where
/// the number of components is not known until runtime. This is problematic
/// for normal `ArrayHandle`s because you have to specify the size of the `Vec`s
/// as a template parameter at compile time. `ArrayHandleRuntimeVec` can be used
/// in this case.
///
/// Note that caution should be used with `ArrayHandleRuntimeVec` because the
/// size of the `Vec` values is not known at compile time. Thus, the value
/// type of this array is forced to a special `VecFromPortal` class that can cause
/// surprises if treated as a `Vec`. In particular, the static `NUM_COMPONENTS`
/// expression does not exist. Furthermore, new variables of type `VecFromPortal`
/// cannot be created. This means that simple operators like `+` will not work
/// because they require an intermediate object to be created. (Equal operators
/// like `+=` do work because they are given an existing variable to place the
/// output.)
///
/// It is possible to provide an `ArrayHandleBasic` of the same component
/// type as the underlying storage for this array. In this case, the array
/// will be accessed much in the same manner as `ArrayHandleGroupVec`.
///
/// `ArrayHandleRuntimeVec` also allows you to convert the array to an
/// `ArrayHandleBasic` of the appropriate `Vec` type (or `component` type).
/// A runtime check will be performed to make sure the number of components
/// matches.
///
template <typename ComponentType>
class ArrayHandleRuntimeVec
  : public vtkm::cont::ArrayHandle<
      vtkm::VecFromPortal<typename ArrayHandleBasic<ComponentType>::WritePortalType>,
      vtkm::cont::StorageTagRuntimeVec>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleRuntimeVec,
    (ArrayHandleRuntimeVec<ComponentType>),
    (vtkm::cont::ArrayHandle<
      vtkm::VecFromPortal<typename ArrayHandleBasic<ComponentType>::WritePortalType>,
      vtkm::cont::StorageTagRuntimeVec>));

private:
  using ComponentsArrayType = vtkm::cont::ArrayHandle<ComponentType, StorageTagBasic>;

public:
  /// @brief Construct an `ArrayHandleRuntimeVec` with a given number of components.
  ///
  /// @param  numComponents The size of the `Vec`s stored in the array. This must be
  /// specified at the time of construction.
  ///
  /// @param componentsArray This optional parameter allows you to supply a basic array
  /// that holds the components. This provides a mechanism to group consecutive values
  /// into vectors.
  VTKM_CONT
  ArrayHandleRuntimeVec(vtkm::IdComponent numComponents,
                        const ComponentsArrayType& componentsArray = ComponentsArrayType{})
    : Superclass(StorageType::CreateBuffers(numComponents, componentsArray))
  {
  }

  /// @brief Return the number of components in each vec value.
  VTKM_CONT vtkm::IdComponent GetNumberOfComponents() const
  {
    return StorageType::GetNumberOfComponents(this->GetBuffers());
  }

  /// @brief Return a basic array containing the components stored in this array.
  ///
  /// The returned array is shared with this object. Modifying the contents of one array
  /// will modify the other.
  VTKM_CONT vtkm::cont::ArrayHandleBasic<ComponentType> GetComponentsArray() const
  {
    return StorageType::GetComponentsArray(this->GetBuffers());
  }

  /// @brief Converts the array to that of a basic array handle.
  ///
  /// This method converts the `ArrayHandleRuntimeVec` to a simple `ArrayHandleBasic`.
  /// This is useful if the `ArrayHandleRuntimeVec` is passed to a routine that works
  /// on an array of a specific `Vec` size (or scalars). After a runtime check, the
  /// array can be converted to a typical array and used as such.
  template <typename ValueType>
  void AsArrayHandleBasic(vtkm::cont::ArrayHandle<ValueType>& array) const
  {
    StorageType::AsArrayHandleBasic(this->GetBuffers(), array);
  }

  /// @copydoc AsArrayHandleBasic
  template <typename ArrayType>
  ArrayType AsArrayHandleBasic() const
  {
    ArrayType array;
    this->AsArrayHandleBasic(array);
    return array;
  }
};

/// `make_ArrayHandleRuntimeVec` is convenience function to generate an
/// `ArrayHandleRuntimeVec`. It takes the number of components stored in
/// each value's `Vec`, which must be specified on the construction of
/// the `ArrayHandleRuntimeVec`. If not specified, the number of components
/// is set to 1. `make_ArrayHandleRuntimeVec` can also optionally take an
/// existing array of components, which will be grouped into `Vec` values
/// based on the specified number of components.
///
template <typename T>
VTKM_CONT auto make_ArrayHandleRuntimeVec(
  vtkm::IdComponent numComponents,
  const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& componentsArray =
    vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>{})
{
  using UnrolledVec = vtkm::internal::UnrollVec<T>;
  using ComponentType = typename UnrolledVec::ComponentType;

  // Use some dangerous magic to convert the basic array to its base component and create
  // an ArrayHandleRuntimeVec from that.
  vtkm::cont::ArrayHandle<ComponentType, vtkm::cont::StorageTagBasic> flatComponents(
    componentsArray.GetBuffers());

  return vtkm::cont::ArrayHandleRuntimeVec<ComponentType>(
    numComponents * UnrolledVec::NUM_COMPONENTS, flatComponents);
}

/// Converts a basic array handle into an `ArrayHandleRuntimeVec` with 1 component. The
/// constructed array is essentially equivalent but of a different type.
template <typename T>
VTKM_CONT auto make_ArrayHandleRuntimeVec(
  const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& componentsArray)
{
  return make_ArrayHandleRuntimeVec(1, componentsArray);
}

/// A convenience function for creating an `ArrayHandleRuntimeVec` from a standard C array.
///
template <typename T>
VTKM_CONT auto make_ArrayHandleRuntimeVec(vtkm::IdComponent numComponents,
                                          const T* array,
                                          vtkm::Id numberOfValues,
                                          vtkm::CopyFlag copy)
{
  return make_ArrayHandleRuntimeVec(numComponents,
                                    vtkm::cont::make_ArrayHandle(array, numberOfValues, copy));
}

/// A convenience function to move a user-allocated array into an `ArrayHandleRuntimeVec`.
/// The provided array pointer will be reset to `nullptr`.
/// If the array was not allocated with the `new[]` operator, then deleter and reallocater
/// functions must be provided.
///
template <typename T>
VTKM_CONT auto make_ArrayHandleRuntimeVecMove(
  vtkm::IdComponent numComponents,
  T*& array,
  vtkm::Id numberOfValues,
  vtkm::cont::internal::BufferInfo::Deleter deleter = internal::SimpleArrayDeleter<T>,
  vtkm::cont::internal::BufferInfo::Reallocater reallocater = internal::SimpleArrayReallocater<T>)
{
  return make_ArrayHandleRuntimeVec(
    numComponents, vtkm::cont::make_ArrayHandleMove(array, numberOfValues, deleter, reallocater));
}

/// A convenience function for creating an `ArrayHandleRuntimeVec` from an `std::vector`.
///
template <typename T, typename Allocator>
VTKM_CONT auto make_ArrayHandleRuntimeVec(vtkm::IdComponent numComponents,
                                          const std::vector<T, Allocator>& array,
                                          vtkm::CopyFlag copy)
{
  return make_ArrayHandleRuntimeVec(numComponents, vtkm::cont::make_ArrayHandle(array, copy));
}

/// Move an `std::vector` into an `ArrayHandleRuntimeVec`.
///
template <typename T, typename Allocator>
VTKM_CONT auto make_ArrayHandleRuntimeVecMove(vtkm::IdComponent numComponents,
                                              std::vector<T, Allocator>&& array)
{
  return make_ArrayHandleRuntimeVec(numComponents, make_ArrayHandleMove(std::move(array)));
}

template <typename T, typename Allocator>
VTKM_CONT auto make_ArrayHandleRuntimeVec(vtkm::IdComponent numComponents,
                                          std::vector<T, Allocator>&& array,
                                          vtkm::CopyFlag vtkmNotUsed(copy))
{
  return make_ArrayHandleRuntimeVecMove(numComponents, std::move(array));
}

namespace internal
{

template <>
struct ArrayExtractComponentImpl<vtkm::cont::StorageTagRuntimeVec>
{
  template <typename T>
  auto operator()(const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagRuntimeVec>& src,
                  vtkm::IdComponent componentIndex,
                  vtkm::CopyFlag allowCopy) const
  {
    using ComponentType = typename T::ComponentType;
    vtkm::cont::ArrayHandleRuntimeVec<ComponentType> array{ src };
    constexpr vtkm::IdComponent NUM_SUB_COMPONENTS = vtkm::VecFlat<ComponentType>::NUM_COMPONENTS;
    vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType> dest =
      ArrayExtractComponentImpl<vtkm::cont::StorageTagBasic>{}(
        array.GetComponentsArray(), componentIndex % NUM_SUB_COMPONENTS, allowCopy);

    // Adjust stride and offset to expectations of grouped values
    const vtkm::IdComponent numComponents = array.GetNumberOfComponents();
    return vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType>(
      dest.GetBasicArray(),
      dest.GetNumberOfValues() / numComponents,
      dest.GetStride() * numComponents,
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

template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandleRuntimeVec<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_RuntimeVec<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename VecType>
struct SerializableTypeString<vtkm::cont::ArrayHandle<VecType, vtkm::cont::StorageTagRuntimeVec>>
  : SerializableTypeString<vtkm::cont::ArrayHandleRuntimeVec<typename VecType::ComponentType>>
{
};

}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleRuntimeVec<T>>
{
private:
  using Type = vtkm::cont::ArrayHandleRuntimeVec<T>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, Type(obj).GetNumberOfComponents());
    vtkmdiy::save(bb, Type(obj).GetComponentsArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    vtkm::IdComponent numComponents;
    vtkm::cont::ArrayHandleBasic<T> componentArray;

    vtkmdiy::load(bb, numComponents);
    vtkmdiy::load(bb, componentArray);

    obj = vtkm::cont::make_ArrayHandleRuntimeVec(numComponents, componentArray);
  }
};

template <typename VecType>
struct Serialization<vtkm::cont::ArrayHandle<VecType, vtkm::cont::StorageTagRuntimeVec>>
  : Serialization<vtkm::cont::ArrayHandleRuntimeVec<typename VecType::ComponentType>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleRuntimeVec_h
