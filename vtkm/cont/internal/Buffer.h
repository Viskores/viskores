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
namespace cont
{
namespace internal
{

namespace detail
{

struct BufferHelper;

} // namespace detail

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
  VTKM_CONT Buffer(Buffer&& src);

  VTKM_CONT ~Buffer();

  VTKM_CONT Buffer& operator=(const Buffer& src);
  VTKM_CONT Buffer& operator=(Buffer&& src);

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
  VTKM_CONT void SetNumberOfBytes(vtkm::BufferSizeType numberOfBytes, vtkm::CopyFlag preserve);

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
  /// \brief Copies the data from this buffer to the target buffer.
  ///
  /// If a device is given, then the copy will be preferred for that device. Otherwise, a device
  /// already containing the data will be used for the copy. If no such device exists, the host
  /// will be used.
  ///
  VTKM_CONT void DeepCopy(vtkm::cont::internal::Buffer& dest) const;
  VTKM_CONT void DeepCopy(vtkm::cont::internal::Buffer& dest,
                          vtkm::cont::DeviceAdapterId device) const;
  /// @}

  /// \brief Resets the `Buffer` to the memory allocated at the information.
  ///
  /// The `Buffer` is initialized to a state that contains the given `buffer` of data. The
  /// `BufferInfo` object self-describes the pointer, size, and device of the memory.
  ///
  VTKM_CONT void Reset(const vtkm::cont::internal::BufferInfo& buffer);

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
