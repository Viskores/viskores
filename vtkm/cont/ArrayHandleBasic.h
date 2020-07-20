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
#include <vtkm/cont/SerializableTypeString.h>
#include <vtkm/cont/Serialization.h>
#include <vtkm/cont/Storage.h>

#include <vtkm/internal/ArrayPortalBasic.h>

#include <limits>

namespace vtkm
{
namespace cont
{

namespace internal
{

namespace detail
{

VTKM_CONT_EXPORT VTKM_CONT vtkm::BufferSizeType NumberOfBytes(vtkm::Id numValues,
                                                              std::size_t typeSize);

} // namespace detail

VTKM_CONT_EXPORT VTKM_CONT void InvalidRealloc(void*&,
                                               void*&,
                                               vtkm::BufferSizeType,
                                               vtkm::BufferSizeType);

template <typename T>
class VTKM_ALWAYS_EXPORT Storage<T, vtkm::cont::StorageTagBasic>
{
public:
  static constexpr vtkm::IdComponent NUMBER_OF_BUFFERS = 1;

  using ReadPortalType = vtkm::internal::ArrayPortalBasicRead<T>;
  using WritePortalType = vtkm::internal::ArrayPortalBasicWrite<T>;

  VTKM_CONT void ResizeBuffers(vtkm::Id numValues,
                               vtkm::cont::internal::Buffer* buffers,
                               vtkm::CopyFlag preserve,
                               vtkm::cont::Token& token)
  {
    buffers[0].SetNumberOfBytes(detail::NumberOfBytes(numValues, sizeof(T)), preserve, token);
  }

  VTKM_CONT vtkm::Id GetNumberOfValues(const vtkm::cont::internal::Buffer* buffers)
  {
    return static_cast<vtkm::Id>(buffers->GetNumberOfBytes()) / static_cast<vtkm::Id>(sizeof(T));
  }

  VTKM_CONT ReadPortalType CreateReadPortal(const vtkm::cont::internal::Buffer* buffers,
                                            vtkm::cont::DeviceAdapterId device,
                                            vtkm::cont::Token& token)
  {
    return ReadPortalType(reinterpret_cast<const T*>(buffers[0].ReadPointerDevice(device, token)),
                          this->GetNumberOfValues(buffers));
  }

  VTKM_CONT WritePortalType CreateWritePortal(const vtkm::cont::internal::Buffer* buffers,
                                              vtkm::cont::DeviceAdapterId device,
                                              vtkm::cont::Token& token)
  {
    return WritePortalType(reinterpret_cast<T*>(buffers[0].WritePointerDevice(device, token)),
                           this->GetNumberOfValues(buffers));
  }
};

} // namespace internal

// This can go away once ArrayHandle is replaced with ArrayHandleNewStyle
template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandle<T, vtkm::cont::StorageTagBasic>
  : public ArrayHandleNewStyle<T, vtkm::cont::StorageTagBasic>
{
  using Superclass = ArrayHandleNewStyle<T, vtkm::cont::StorageTagBasic>;

public:
  VTKM_CONT
  ArrayHandle()
    : Superclass()
  {
  }

  VTKM_CONT
  ArrayHandle(const ArrayHandle<T, vtkm::cont::StorageTagBasic>& src)
    : Superclass(src)
  {
  }

  VTKM_CONT
  ArrayHandle(ArrayHandle<T, vtkm::cont::StorageTagBasic>&& src) noexcept
    : Superclass(std::move(src))
  {
  }

  VTKM_CONT
  ArrayHandle(const ArrayHandleNewStyle<T, vtkm::cont::StorageTagBasic>& src)
    : Superclass(src)
  {
  }

  VTKM_CONT
  ArrayHandle(ArrayHandleNewStyle<T, vtkm::cont::StorageTagBasic>&& src) noexcept
    : Superclass(std::move(src))
  {
  }

  VTKM_CONT ArrayHandle(
    const vtkm::cont::internal::Buffer* buffers,
    const typename Superclass::StorageType& storage = typename Superclass::StorageType())
    : Superclass(buffers, storage)
  {
  }

  VTKM_CONT ArrayHandle(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    const typename Superclass::StorageType& storage = typename Superclass::StorageType())
    : Superclass(buffers, storage)
  {
  }

  VTKM_CONT
  ArrayHandle<T, vtkm::cont::StorageTagBasic>& operator=(
    const ArrayHandle<T, vtkm::cont::StorageTagBasic>& src)
  {
    this->Superclass::operator=(src);
    return *this;
  }

  VTKM_CONT
  ArrayHandle<T, vtkm::cont::StorageTagBasic>& operator=(
    ArrayHandle<T, vtkm::cont::StorageTagBasic>&& src) noexcept
  {
    this->Superclass::operator=(std::move(src));
    return *this;
  }

  VTKM_CONT ~ArrayHandle() {}
};

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
    : Superclass(std::vector<vtkm::cont::internal::Buffer>{
        vtkm::cont::internal::MakeBuffer(vtkm::cont::DeviceAdapterTagUndefined{},
                                         array,
                                         array,
                                         internal::detail::NumberOfBytes(numberOfValues, sizeof(T)),
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
    : Superclass(std::vector<vtkm::cont::internal::Buffer>{
        vtkm::cont::internal::MakeBuffer(device,
                                         array,
                                         array,
                                         internal::detail::NumberOfBytes(numberOfValues, sizeof(T)),
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
    : Superclass(std::vector<vtkm::cont::internal::Buffer>{
        vtkm::cont::internal::MakeBuffer(vtkm::cont::DeviceAdapterTagUndefined{},
                                         array,
                                         container,
                                         internal::detail::NumberOfBytes(numberOfValues, sizeof(T)),
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
    : Superclass(std::vector<vtkm::cont::internal::Buffer>{
        vtkm::cont::internal::MakeBuffer(device,
                                         array,
                                         container,
                                         internal::detail::NumberOfBytes(numberOfValues, sizeof(T)),
                                         deleter,
                                         reallocater) })
  {
  }

  /// @{
  /// \brief Gets raw access to the `ArrayHandle`'s data.
  ///
  /// Note that the returned array may become invalidated by other operations on the ArryHandle
  /// unless you provide a token.
  ///
  const T* GetReadPointer(vtkm::cont::Token& token) const
  {
    return reinterpret_cast<const T*>(this->GetBuffers()[0].ReadPointerHost(token));
  }
  const T* GetReadPointer() const
  {
    vtkm::cont::Token token;
    return this->GetReadPointer(token);
  }
  T* GetWritePointer(vtkm::cont::Token& token) const
  {
    return reinterpret_cast<T*>(this->GetBuffers()[0].WritePointerHost(token));
  }
  T* GetWritePointer() const
  {
    vtkm::cont::Token token;
    return this->GetWritePointer(token);
  }

  const T* GetReadPointer(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token& token) const
  {
    return reinterpret_cast<const T*>(this->GetBuffers()[0].ReadPointerDevice(device, token));
  }
  const T* GetReadPointer(vtkm::cont::DeviceAdapterId device) const
  {
    vtkm::cont::Token token;
    return this->GetReadPointer(device, token);
  }
  T* GetWritePointer(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token& token) const
  {
    return reinterpret_cast<T*>(this->GetBuffers()[0].WritePointerDevice(device, token));
  }
  T* GetWritePointer(vtkm::cont::DeviceAdapterId device) const
  {
    vtkm::cont::Token token;
    return this->GetWritePointer(device, token);
  }
  /// @}
};

namespace internal
{

// Deletes a container object by casting it to a pointer of a given type (the template argument)
// and then using delete[] on the object.
template <typename T>
VTKM_CONT inline void SimpleArrayDeleter(void* container_)
{
  T* container = reinterpret_cast<T*>(container_);
  delete[] container;
}

// Reallocates a standard C array. Note that the allocation method is different than the default
// host allocation of vtkm::cont::internal::BufferInfo and may be less efficient.
template <typename T>
VTKM_CONT inline void SimpleArrayReallocater(void*& memory,
                                             void*& container,
                                             vtkm::BufferSizeType oldSize,
                                             vtkm::BufferSizeType newSize)
{
  VTKM_ASSERT(memory == container);
  VTKM_ASSERT(static_cast<std::size_t>(newSize) % sizeof(T) == 0);

  // If the new size is not much smaller than the old size, just reuse the buffer (and waste a
  // little memory).
  if ((newSize > ((3 * oldSize) / 4)) && (newSize <= oldSize))
  {
    return;
  }

  void* newBuffer = new T[static_cast<std::size_t>(newSize) / sizeof(T)];
  std::memcpy(newBuffer, memory, static_cast<std::size_t>(newSize < oldSize ? newSize : oldSize));

  if (memory != nullptr)
  {
    SimpleArrayDeleter<T>(memory);
  }

  memory = container = newBuffer;
}

// Deletes a container object by casting it to a pointer of a given type (the template argument)
// and then using delete on the object.
template <typename T>
VTKM_CONT inline void CastDeleter(void* container_)
{
  T* container = reinterpret_cast<T*>(container_);
  delete container;
}

template <typename T, typename Allocator>
VTKM_CONT inline void StdVectorDeleter(void* container)
{
  CastDeleter<std::vector<T, Allocator>>(container);
}

template <typename T, typename Allocator>
VTKM_CONT inline void StdVectorReallocater(void*& memory,
                                           void*& container,
                                           vtkm::BufferSizeType oldSize,
                                           vtkm::BufferSizeType newSize)
{
  using vector_type = std::vector<T, Allocator>;
  vector_type* vector = reinterpret_cast<vector_type*>(container);
  VTKM_ASSERT(vector->empty() || (memory == vector->data()));
  VTKM_ASSERT(oldSize == static_cast<vtkm::BufferSizeType>(vector->size()));

  vector->resize(static_cast<std::size_t>(newSize));
  memory = vector->data();
}

} // namespace internal

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

template <typename T>
VTKM_DEPRECATED(1.6, "Specify a vtkm::CopyFlag or use a move version of make_ArrayHandle.")
VTKM_CONT vtkm::cont::ArrayHandleBasic<T> make_ArrayHandle(const T* array, vtkm::Id numberOfValues)
{
  return make_ArrayHandle(array, numberOfValues, vtkm::CopyFlag::Off);
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

template <typename T, typename Allocator>
VTKM_DEPRECATED(1.6, "Specify a vtkm::CopyFlag or use a move version of make_ArrayHandle.")
VTKM_CONT vtkm::cont::ArrayHandleBasic<T> make_ArrayHandle(const std::vector<T, Allocator>& array)
{
  return make_ArrayHandle(array, vtkm::CopyFlag::Off);
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

template <typename T, typename Allocator>
VTKM_CONT vtkm::cont::ArrayHandleBasic<T> make_ArrayHandle(std::vector<T, Allocator>&& array,
                                                           vtkm::CopyFlag copy)
{
  if (copy == vtkm::CopyFlag::On)
  {
    VTKM_LOG_S(
      vtkm::cont::LogLevel::Info,
      "CopyFlag states std::vector should be copied, but it can be moved. Ignoring copy flag.");
  }
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

    obj = vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>(&buffer);
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

namespace vtkm
{
namespace cont
{

namespace internal
{

/// \cond
/// Make doxygen ignore this section
#define VTKM_STORAGE_EXPORT(Type)                                                                  \
  extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<Type, StorageTagBasic>;                  \
  extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Vec<Type, 2>, StorageTagBasic>;    \
  extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Vec<Type, 3>, StorageTagBasic>;    \
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
/// \endcond

} // namespace internal

#define VTKM_ARRAYHANDLE_EXPORT(Type)                                                              \
  extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandleNewStyle<Type, StorageTagBasic>;      \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayHandleNewStyle<vtkm::Vec<Type, 2>, StorageTagBasic>;                                      \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayHandleNewStyle<vtkm::Vec<Type, 3>, StorageTagBasic>;                                      \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayHandleNewStyle<vtkm::Vec<Type, 4>, StorageTagBasic>;

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

#endif // !vtk_m_cont_ArrayHandleBasic_cxx

#endif //vtk_m_cont_ArrayHandleBasic_h
