//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleBasic_h
#define vtk_m_cont_ArrayHandleBasic_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/SerializableTypeString.h>
#include <vtkm/cont/Serialization.h>
#include <vtkm/cont/Storage.h>

#include <vtkm/internal/ArrayPortalBasic.h>

#include <vtkm/VecFlat.h>

#include <limits>

namespace vtkm
{
namespace cont
{

namespace internal
{

template <typename T>
class VTKM_ALWAYS_EXPORT Storage<T, vtkm::cont::StorageTagBasic>
{
public:
  using ReadPortalType = vtkm::internal::ArrayPortalBasicRead<T>;
  using WritePortalType = vtkm::internal::ArrayPortalBasicWrite<T>;

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers()
  {
    return std::vector<vtkm::cont::internal::Buffer>(1);
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token)
  {
    buffers[0].SetNumberOfBytes(
      vtkm::internal::NumberOfValuesToNumberOfBytes<T>(numValues), preserve, token);
  }

  VTKM_CONT static vtkm::IdComponent GetNumberOfComponentsFlat(
    const std::vector<vtkm::cont::internal::Buffer>&)
  {
    return vtkm::VecFlat<T>::NUM_COMPONENTS;
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    VTKM_ASSERT(buffers.size() == 1);
    return static_cast<vtkm::Id>(buffers[0].GetNumberOfBytes() /
                                 static_cast<vtkm::BufferSizeType>(sizeof(T)));
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>& buffers,
                             const T& fillValue,
                             vtkm::Id startIndex,
                             vtkm::Id endIndex,
                             vtkm::cont::Token& token)
  {
    VTKM_ASSERT(buffers.size() == 1);
    constexpr vtkm::BufferSizeType fillValueSize =
      static_cast<vtkm::BufferSizeType>(sizeof(fillValue));
    buffers[0].Fill(
      &fillValue, fillValueSize, startIndex * fillValueSize, endIndex * fillValueSize, token);
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    VTKM_ASSERT(buffers.size() == 1);
    return ReadPortalType(reinterpret_cast<const T*>(buffers[0].ReadPointerDevice(device, token)),
                          GetNumberOfValues(buffers));
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    VTKM_ASSERT(buffers.size() == 1);
    return WritePortalType(reinterpret_cast<T*>(buffers[0].WritePointerDevice(device, token)),
                           GetNumberOfValues(buffers));
  }
};

} // namespace internal

/// @brief Basic array storage for an array handle.
///
/// This array handle references a standard C array. It provides a level
/// of safety and management across devices.
/// This is the default used when no storage is specified. Using this subclass
/// allows access to the underlying raw array.
template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandleBasic : public ArrayHandle<T, vtkm::cont::StorageTagBasic>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleBasic,
                             (ArrayHandleBasic<T>),
                             (ArrayHandle<T, vtkm::cont::StorageTagBasic>));

  ArrayHandleBasic(
    T* array,
    vtkm::Id numberOfValues,
    vtkm::cont::internal::BufferInfo::Deleter deleter,
    vtkm::cont::internal::BufferInfo::Reallocater reallocater = internal::InvalidRealloc)
    : Superclass(std::vector<vtkm::cont::internal::Buffer>{ vtkm::cont::internal::MakeBuffer(
        vtkm::cont::DeviceAdapterTagUndefined{},
        array,
        array,
        vtkm::internal::NumberOfValuesToNumberOfBytes<T>(numberOfValues),
        deleter,
        reallocater) })
  {
  }

  ArrayHandleBasic(
    T* array,
    vtkm::Id numberOfValues,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::internal::BufferInfo::Deleter deleter,
    vtkm::cont::internal::BufferInfo::Reallocater reallocater = internal::InvalidRealloc)
    : Superclass(std::vector<vtkm::cont::internal::Buffer>{ vtkm::cont::internal::MakeBuffer(
        device,
        array,
        array,
        vtkm::internal::NumberOfValuesToNumberOfBytes<T>(numberOfValues),
        deleter,
        reallocater) })
  {
  }

  ArrayHandleBasic(
    T* array,
    void* container,
    vtkm::Id numberOfValues,
    vtkm::cont::internal::BufferInfo::Deleter deleter,
    vtkm::cont::internal::BufferInfo::Reallocater reallocater = internal::InvalidRealloc)
    : Superclass(std::vector<vtkm::cont::internal::Buffer>{ vtkm::cont::internal::MakeBuffer(
        vtkm::cont::DeviceAdapterTagUndefined{},
        array,
        container,
        vtkm::internal::NumberOfValuesToNumberOfBytes<T>(numberOfValues),
        deleter,
        reallocater) })
  {
  }

  ArrayHandleBasic(
    T* array,
    void* container,
    vtkm::Id numberOfValues,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::internal::BufferInfo::Deleter deleter,
    vtkm::cont::internal::BufferInfo::Reallocater reallocater = internal::InvalidRealloc)
    : Superclass(std::vector<vtkm::cont::internal::Buffer>{ vtkm::cont::internal::MakeBuffer(
        device,
        array,
        container,
        vtkm::internal::NumberOfValuesToNumberOfBytes<T>(numberOfValues),
        deleter,
        reallocater) })
  {
  }

  /// @brief Gets raw access to the `ArrayHandle`'s data.
  ///
  /// Note that the returned array may become invalidated by other operations on the ArryHandle.
  ///
  const T* GetReadPointer() const
  {
    vtkm::cont::Token token;
    return this->GetReadPointer(token);
  }
  /// @brief Gets raw access to the `ArrayHandle`'s data.
  ///
  /// @param token When a `vtkm::cont::Token` is provided, the array is locked
  /// from being used by any write operations until the token goes out of scope.
  ///
  const T* GetReadPointer(vtkm::cont::Token& token) const
  {
    return reinterpret_cast<const T*>(this->GetBuffers()[0].ReadPointerHost(token));
  }
  /// @brief Gets raw write access to the `ArrayHandle`'s data.
  ///
  /// Note that the returned array may become invalidated by other operations on the ArryHandle.
  ///
  T* GetWritePointer() const
  {
    vtkm::cont::Token token;
    return this->GetWritePointer(token);
  }
  /// @brief Gets raw write access to the `ArrayHandle`'s data.
  ///
  /// @param token When a `vtkm::cont::Token` is provided, the array is locked
  /// from being used by any read or write operations until the token goes out of scope.
  ///
  T* GetWritePointer(vtkm::cont::Token& token) const
  {
    return reinterpret_cast<T*>(this->GetBuffers()[0].WritePointerHost(token));
  }

  /// @brief Gets raw access to the `ArrayHandle`'s data on a particular device.
  ///
  /// Note that the returned array may become invalidated by other operations on the ArryHandle.
  ///
  /// @param device The device ID or device tag specifying on which device the array will
  /// be valid on.
  ///
  const T* GetReadPointer(vtkm::cont::DeviceAdapterId device) const
  {
    vtkm::cont::Token token;
    return this->GetReadPointer(device, token);
  }
  /// @brief Gets raw access to the `ArrayHandle`'s data.
  ///
  /// @param device The device ID or device tag specifying on which device the array will
  /// be valid on.
  /// @param token When a `vtkm::cont::Token` is provided, the array is locked
  /// from being used by any write operations until the token goes out of scope.
  ///
  const T* GetReadPointer(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token& token) const
  {
    return reinterpret_cast<const T*>(this->GetBuffers()[0].ReadPointerDevice(device, token));
  }
  /// @brief Gets raw write access to the `ArrayHandle`'s data.
  ///
  /// Note that the returned array may become invalidated by other operations on the ArryHandle.
  ///
  /// @param device The device ID or device tag specifying on which device the array will
  /// be valid on.
  ///
  T* GetWritePointer(vtkm::cont::DeviceAdapterId device) const
  {
    vtkm::cont::Token token;
    return this->GetWritePointer(device, token);
  }
  /// @brief Gets raw write access to the `ArrayHandle`'s data.
  ///
  /// @param device The device ID or device tag specifying on which device the array will
  /// be valid on.
  /// @param token When a `vtkm::cont::Token` is provided, the array is locked
  /// from being used by any read or write operations until the token goes out of scope.
  ///
  T* GetWritePointer(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token& token) const
  {
    return reinterpret_cast<T*>(this->GetBuffers()[0].WritePointerDevice(device, token));
  }
};

/// A convenience function for creating an ArrayHandle from a standard C array.
///
template <typename T>
VTKM_CONT vtkm::cont::ArrayHandleBasic<T> make_ArrayHandle(const T* array,
                                                           vtkm::Id numberOfValues,
                                                           vtkm::CopyFlag copy)
{
  if (copy == vtkm::CopyFlag::On)
  {
    vtkm::cont::ArrayHandleBasic<T> handle;
    handle.Allocate(numberOfValues);
    std::copy(
      array, array + numberOfValues, vtkm::cont::ArrayPortalToIteratorBegin(handle.WritePortal()));
    return handle;
  }
  else
  {
    return vtkm::cont::ArrayHandleBasic<T>(const_cast<T*>(array), numberOfValues, [](void*) {});
  }
}

/// A convenience function to move a user-allocated array into an `ArrayHandle`.
/// The provided array pointer will be reset to `nullptr`.
/// If the array was not allocated with the `new[]` operator, then deleter and reallocater
/// functions must be provided.
///
template <typename T>
VTKM_CONT vtkm::cont::ArrayHandleBasic<T> make_ArrayHandleMove(
  T*& array,
  vtkm::Id numberOfValues,
  vtkm::cont::internal::BufferInfo::Deleter deleter = internal::SimpleArrayDeleter<T>,
  vtkm::cont::internal::BufferInfo::Reallocater reallocater = internal::SimpleArrayReallocater<T>)
{
  vtkm::cont::ArrayHandleBasic<T> arrayHandle(array, numberOfValues, deleter, reallocater);
  array = nullptr;
  return arrayHandle;
}

/// A convenience function for creating an ArrayHandle from an std::vector.
///
template <typename T, typename Allocator>
VTKM_CONT vtkm::cont::ArrayHandleBasic<T> make_ArrayHandle(const std::vector<T, Allocator>& array,
                                                           vtkm::CopyFlag copy)
{
  if (!array.empty())
  {
    return make_ArrayHandle(array.data(), static_cast<vtkm::Id>(array.size()), copy);
  }
  else
  {
    // Vector empty. Just return an empty array handle.
    return vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>();
  }
}

/// Move an std::vector into an ArrayHandle.
///
template <typename T, typename Allocator>
VTKM_CONT vtkm::cont::ArrayHandleBasic<T> make_ArrayHandleMove(std::vector<T, Allocator>&& array)
{
  using vector_type = std::vector<T, Allocator>;
  vector_type* container = new vector_type(std::move(array));
  return vtkm::cont::ArrayHandleBasic<T>(container->data(),
                                         container,
                                         static_cast<vtkm::Id>(container->size()),
                                         internal::StdVectorDeleter<T, Allocator>,
                                         internal::StdVectorReallocater<T, Allocator>);
}

/// Move an std::vector into an ArrayHandle.
///
template <typename T, typename Allocator>
VTKM_CONT vtkm::cont::ArrayHandleBasic<T> make_ArrayHandle(std::vector<T, Allocator>&& array,
                                                           vtkm::CopyFlag vtkmNotUsed(copy))
{
  return make_ArrayHandleMove(array);
}

/// Create an ArrayHandle directly from an initializer list of values.
///
template <typename T>
VTKM_CONT vtkm::cont::ArrayHandleBasic<T> make_ArrayHandle(std::initializer_list<T>&& values)
{
  return make_ArrayHandle(values.begin(), static_cast<vtkm::Id>(values.size()), vtkm::CopyFlag::On);
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
struct SerializableTypeString<vtkm::cont::ArrayHandleBasic<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<ArrayHandle<T, vtkm::cont::StorageTagBasic>>
  : SerializableTypeString<vtkm::cont::ArrayHandleBasic<T>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleBasic<T>>
{
  static VTKM_CONT void save(BinaryBuffer& bb,
                             const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& obj)
  {
    vtkmdiy::save(bb, obj.GetBuffers()[0]);
  }

  static VTKM_CONT void load(BinaryBuffer& bb,
                             vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& obj)
  {
    vtkm::cont::internal::Buffer buffer;
    vtkmdiy::load(bb, buffer);

    obj = vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>(
      vtkm::cont::internal::CreateBuffers(buffer));
  }
};

template <typename T>
struct Serialization<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>>
  : Serialization<vtkm::cont::ArrayHandleBasic<T>>
{
};

} // diy
/// @endcond SERIALIZATION

#ifndef vtk_m_cont_ArrayHandleBasic_cxx

/// @cond
/// Make doxygen ignore this section

namespace vtkm
{
namespace cont
{

namespace internal
{

#define VTKM_STORAGE_EXPORT(Type)                                                               \
  extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<Type, StorageTagBasic>;               \
  extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Vec<Type, 2>, StorageTagBasic>; \
  extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Vec<Type, 3>, StorageTagBasic>; \
  extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Vec<Type, 4>, StorageTagBasic>;

VTKM_STORAGE_EXPORT(char)
VTKM_STORAGE_EXPORT(vtkm::Int8)
VTKM_STORAGE_EXPORT(vtkm::UInt8)
VTKM_STORAGE_EXPORT(vtkm::Int16)
VTKM_STORAGE_EXPORT(vtkm::UInt16)
VTKM_STORAGE_EXPORT(vtkm::Int32)
VTKM_STORAGE_EXPORT(vtkm::UInt32)
VTKM_STORAGE_EXPORT(vtkm::Int64)
VTKM_STORAGE_EXPORT(vtkm::UInt64)
VTKM_STORAGE_EXPORT(vtkm::Float32)
VTKM_STORAGE_EXPORT(vtkm::Float64)

#undef VTKM_STORAGE_EXPORT

} // namespace internal

#define VTKM_ARRAYHANDLE_EXPORT(Type)                                                 \
  extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>; \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                     \
    ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>;                                 \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                     \
    ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>;                                 \
  extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>;

VTKM_ARRAYHANDLE_EXPORT(char)
VTKM_ARRAYHANDLE_EXPORT(vtkm::Int8)
VTKM_ARRAYHANDLE_EXPORT(vtkm::UInt8)
VTKM_ARRAYHANDLE_EXPORT(vtkm::Int16)
VTKM_ARRAYHANDLE_EXPORT(vtkm::UInt16)
VTKM_ARRAYHANDLE_EXPORT(vtkm::Int32)
VTKM_ARRAYHANDLE_EXPORT(vtkm::UInt32)
VTKM_ARRAYHANDLE_EXPORT(vtkm::Int64)
VTKM_ARRAYHANDLE_EXPORT(vtkm::UInt64)
VTKM_ARRAYHANDLE_EXPORT(vtkm::Float32)
VTKM_ARRAYHANDLE_EXPORT(vtkm::Float64)

#undef VTKM_ARRAYHANDLE_EXPORT
}
} // end vtkm::cont

/// @endcond

#endif // !vtk_m_cont_ArrayHandleBasic_cxx

#endif //vtk_m_cont_ArrayHandleBasic_h
