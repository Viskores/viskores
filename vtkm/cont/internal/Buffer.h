//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_vtkm_cont_internal_Buffer_h
#define vtk_m_vtkm_cont_internal_Buffer_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/Serialization.h>
#include <vtkm/cont/Token.h>

#include <vtkm/cont/internal/DeviceAdapterMemoryManager.h>

#include <memory>
#include <mutex>

namespace vtkm
{

namespace internal
{

///@{
/// \brief Convert the number of values of a type to the number of bytes needed to store it.
///
/// A convenience function that takes the number of values in an array and either the type or the
/// size of the type and safely converts that to the number of bytes required to store the array.
///
/// This function can throw an `vtkm::cont::ErrorBadAllocation` if the number of bytes cannot be
/// stored in the returned `vtkm::BufferSizeType`. (That would be a huge array and probably
/// indicative of an error.)
///
VTKM_CONT_EXPORT vtkm::BufferSizeType NumberOfValuesToNumberOfBytes(vtkm::Id numValues,
                                                                    std::size_t typeSize);

template <typename T>
vtkm::BufferSizeType NumberOfValuesToNumberOfBytes(vtkm::Id numValues)
{
  return NumberOfValuesToNumberOfBytes(numValues, sizeof(T));
}
///@}

} // namespace internal

namespace cont
{
namespace internal
{

namespace detail
{

struct BufferHelper;

} // namespace detail

/// \brief An object to hold metadata for a `Buffer` object.
///
/// A `Buffer` object can optionally hold a `BufferMetaData` object. The metadata object
/// allows the buffer to hold state for the buffer that is not directly related to the
/// memory allocated and its size. This allows you to completely encapsulate the state
/// in the `Buffer` object and then pass the `Buffer` object to different object that
/// provide different interfaces to the array.
///
/// To use `BufferMetaData`, create a subclass, and then provide that subclass as the
/// metadata. The `Buffer` object will only remember it as the generic base class. You
/// can then get the metadata and perform a `dynamic_cast` to check that the metadata
/// is as expected and to get to the meta information
///
struct VTKM_CONT_EXPORT BufferMetaData
{
  virtual ~BufferMetaData();

  /// Subclasses must provide a way to deep copy metadata.
  ///
  virtual std::unique_ptr<BufferMetaData> DeepCopy() const = 0;
};

/// \brief Manages a buffer data among the host and various devices.
///
/// The `Buffer` class defines a contiguous section of memory of a specified number of bytes. The
/// data in this buffer is managed in the host and across the devices supported by VTK-m. `Buffer`
/// will allocate memory and transfer data as necessary.
///
class VTKM_CONT_EXPORT Buffer final
{
  class InternalsStruct;
  std::shared_ptr<InternalsStruct> Internals;

  friend struct vtkm::cont::internal::detail::BufferHelper;

public:
  /// \brief Create an empty `Buffer`.
  ///
  VTKM_CONT Buffer();

  VTKM_CONT Buffer(const Buffer& src);
  VTKM_CONT Buffer(Buffer&& src) noexcept;

  VTKM_CONT ~Buffer();

  VTKM_CONT Buffer& operator=(const Buffer& src);
  VTKM_CONT Buffer& operator=(Buffer&& src) noexcept;

  /// \brief Returns the number of bytes held by the buffer.
  ///
  /// Note that `Buffer` allocates memory lazily. So there might not actually be any memory
  /// allocated anywhere. It is also possible that memory is simultaneously allocated on multiple
  /// devices.
  ///
  VTKM_CONT vtkm::BufferSizeType GetNumberOfBytes() const;

  /// \brief Changes the size of the buffer.
  ///
  /// Note that `Buffer` alloates memory lazily. So there might not be any memory allocated at
  /// the return of the call. (However, later calls to retrieve pointers will allocate memory
  /// as necessary.)
  ///
  /// The `preserve` argument flags whether any existing data in the buffer is preserved.
  /// Preserving data might cost more time or memory.
  ///
  VTKM_CONT void SetNumberOfBytes(vtkm::BufferSizeType numberOfBytes,
                                  vtkm::CopyFlag preserve,
                                  vtkm::cont::Token& token);

  /// \brief Gets the metadata for the buffer.
  ///
  /// Holding metadata in a `Buffer` is optional. The metadata is held in a subclass of
  /// `BufferMetaData`, and you will have to safely downcast the object to retrieve the
  /// actual information.
  ///
  /// The metadata could be a `nullptr` if the metadata was never set.
  ///
  VTKM_CONT vtkm::cont::internal::BufferMetaData* GetMetaData() const;

  /// \brief Sets the metadata for the buffer.
  ///
  /// This form of SetMetaData takes an rvalue to a unique_ptr holding the metadata to
  /// ensure that the object is properly managed.
  ///
  VTKM_CONT void SetMetaData(std::unique_ptr<vtkm::cont::internal::BufferMetaData>&& metadata);

  /// \brief Sets the metadata for the buffer.
  ///
  /// This form of SetMetaData takes the metadata object value. The metadata object
  /// must be a subclass of BufferMetaData or you will get a compile error.
  ///
  template <typename MetaDataType>
  VTKM_CONT void SetMetaData(const MetaDataType& metadata)
  {
    this->SetMetaData(
      std::unique_ptr<vtkm::cont::internal::BufferMetaData>(new MetaDataType(metadata)));
  }

  /// \brief Returns `true` if the buffer is allocated on the host.
  ///
  VTKM_CONT bool IsAllocatedOnHost() const;

  /// \brief Returns `true` if the buffer is allocated on the given device.
  ///
  /// If `device` is `DeviceAdapterTagUnknown`, then this returns the same value as
  /// `IsAllocatedOnHost`. If `device` is `DeviceAdapterTagAny`, then this returns true if
  /// allocated on any device.
  ///
  VTKM_CONT bool IsAllocatedOnDevice(vtkm::cont::DeviceAdapterId device) const;

  /// \brief Returns a readable host (control environment) pointer to the buffer.
  ///
  /// Memory will be allocated and data will be copied as necessary. The memory at the pointer will
  /// be valid as long as `token` is still in scope. Any write operation to this buffer will be
  /// blocked until the `token` goes out of scope.
  ///
  VTKM_CONT const void* ReadPointerHost(vtkm::cont::Token& token) const;

  /// \brief Returns a readable device pointer to the buffer.
  ///
  /// Memory will be allocated and data will be copied as necessary. The memory at the pointer will
  /// be valid as long as `token` is still in scope. Any write operation to this buffer will be
  /// blocked until the `token` goes out of scope.
  ///
  /// If `device` is `DeviceAdapterTagUnknown`, then this has the same behavior as
  /// `ReadPointerHost`. It is an error to set `device` to `DeviceAdapterTagAny`.
  ///
  VTKM_CONT const void* ReadPointerDevice(vtkm::cont::DeviceAdapterId device,
                                          vtkm::cont::Token& token) const;

  /// \brief Returns a writable host (control environment) pointer to the buffer.
  ///
  /// Memory will be allocated and data will be copied as necessary. The memory at the pointer will
  /// be valid as long as `token` is still in scope. Any read or write operation to this buffer
  /// will be blocked until the `token` goes out of scope.
  ///
  VTKM_CONT void* WritePointerHost(vtkm::cont::Token& token) const;

  /// \brief Returns a writable device pointer to the buffer.
  ///
  /// Memory will be allocated and data will be copied as necessary. The memory at the pointer will
  /// be valid as long as `token` is still in scope. Any read or write operation to this buffer
  /// will be blocked until the `token` goes out of scope.
  ///
  /// If `device` is `DeviceAdapterTagUnknown`, then this has the same behavior as
  /// `WritePointerHost`. It is an error to set `device` to `DeviceAdapterTagAny`.
  ///
  VTKM_CONT void* WritePointerDevice(vtkm::cont::DeviceAdapterId device,
                                     vtkm::cont::Token& token) const;

  /// \brief Enqueue a token for access to the buffer.
  ///
  /// This method places the given `Token` into the queue of `Token`s waiting for
  /// access to this `Buffer` and then returns immediately. When this token
  /// is later used to get data from this `Buffer` (for example, in a call to
  /// `ReadPointerDevice`), it will use this place in the queue while waiting for
  ///
  /// \warning After calling this method it is required to subsequently call a
  /// method that attaches the token to this `Buffer`. Otherwise, the enqueued
  /// token will block any subsequent access to the `ArrayHandle`, even if the
  /// `Token` is destroyed.
  ///
  VTKM_CONT void Enqueue(const vtkm::cont::Token& token) const;

  /// @{
  /// \brief Copies the data from the provided buffer into this buffer.
  ///
  /// If a device is given, then the copy will be preferred for that device. Otherwise, a device
  /// already containing the data will be used for the copy. If no such device exists, the host
  /// will be used.
  ///
  VTKM_CONT void DeepCopyFrom(const vtkm::cont::internal::Buffer& source) const;
  VTKM_CONT void DeepCopyFrom(const vtkm::cont::internal::Buffer& source,
                              vtkm::cont::DeviceAdapterId device) const;
  /// @}

  /// \brief Resets the `Buffer` to the memory allocated at the `BufferInfo`.
  ///
  /// The `Buffer` is initialized to a state that contains the given `buffer` of data. The
  /// `BufferInfo` object self-describes the pointer, size, and device of the memory.
  ///
  /// The given memory is "pinned" in the `Buffer`. This means that this memory will always
  /// be used on the given host or device. If `SetNumberOfBytes` is later called with a size
  /// that is inconsistent with the size of this buffer, an exception will be thrown.
  ///
  VTKM_CONT void Reset(const vtkm::cont::internal::BufferInfo& buffer);

  /// \brief Unallocates the buffer from all devices.
  ///
  /// This method preserves the data on the host even if the data must be transferred
  /// there.
  ///
  /// Note that this method will not physically deallocate memory on a device that shares
  /// a memory space with the host (since the data must be preserved on the host). This
  /// is true even for memory spaces that page data between host and device. This method
  /// will not attempt to unpage data from a device with shared memory.
  ///
  VTKM_CONT void ReleaseDeviceResources() const;

  /// \brief Gets the `BufferInfo` object to the memory allocated on the host.
  ///
  VTKM_CONT vtkm::cont::internal::BufferInfo GetHostBufferInfo() const;

  /// \brief Gets the `BufferInfo` object to the memory allocated on the given device.
  ///
  /// If the device is `DeviceAdapterTagUndefined`, the pointer for the host is returned. It is
  /// invalid to select `DeviceAdapterTagAny`.
  ///
  VTKM_CONT vtkm::cont::internal::BufferInfo GetDeviceBufferInfo(
    vtkm::cont::DeviceAdapterId device) const;

  /// \brief Transfer ownership of the Host `BufferInfo` from this buffer
  /// to the caller. This is used to allow memory owned by VTK-m to be
  /// transferred to an owner whose lifespan is longer
  VTKM_CONT vtkm::cont::internal::TransferredBuffer TakeHostBufferOwnership();

  /// \brief Transfer ownership of the device `BufferInfo` from this buffer
  /// to the caller. This is used to allow memory owned by VTK-m to be
  /// transferred to an owner whose lifespan is longer
  VTKM_CONT vtkm::cont::internal::TransferredBuffer TakeDeviceBufferOwnership(
    vtkm::cont::DeviceAdapterId device);

  VTKM_CONT bool operator==(const vtkm::cont::internal::Buffer& rhs) const
  {
    return (this->Internals == rhs.Internals);
  }

  VTKM_CONT bool operator!=(const vtkm::cont::internal::Buffer& rhs) const
  {
    return (this->Internals != rhs.Internals);
  }
};

template <typename... ResetArgs>
VTKM_CONT vtkm::cont::internal::Buffer MakeBuffer(ResetArgs&&... resetArgs)
{
  vtkm::cont::internal::Buffer buffer;
  buffer.Reset(vtkm::cont::internal::BufferInfo(std::forward<ResetArgs>(resetArgs)...));
  return buffer;
}
}
}
} // namespace vtkm::cont::internal

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

template <>
struct VTKM_CONT_EXPORT Serialization<vtkm::cont::internal::Buffer>
{
  static VTKM_CONT void save(BinaryBuffer& bb, const vtkm::cont::internal::Buffer& obj);
  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::internal::Buffer& obj);
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_vtkm_cont_internal_Buffer_h
