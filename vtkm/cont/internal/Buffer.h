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
class VTKM_CONT_EXPORT Buffer
{
  class InternalsStruct;
  std::shared_ptr<InternalsStruct> Internals;

  friend struct vtkm::cont::internal::detail::BufferHelper;

public:
  /// \brief Create an empty `Buffer`.
  ///
  VTKM_CONT Buffer();

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
  VTKM_CONT void* WritePointerDevice(vtkm::cont::DeviceAdapterId device,
                                     vtkm::cont::Token& token) const;

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

  /// \brief Resets the `Buffer` to the memory allocated at the given pointer.
  ///
  /// The `Buffer` is initialized to a state that contains the given `buffer` of data. The
  /// provided `shared_ptr` should have a deleter that appropriately deletes the buffer when
  /// it is no longer used.
  ///
  VTKM_CONT void Reset(const std::shared_ptr<vtkm::UInt8> buffer,
                       vtkm::BufferSizeType numberOfBytes);

  /// \brief Resets the `Buffer` to the memory allocated at the given pointer on a device.
  ///
  /// The `Buffer` is initialized to a state that contains the given `buffer` of data. The pointer
  /// is assumed to be memory in the given `device`. The provided `shared_ptr` should have a
  /// deleter that appropriately deletes the buffer when it is no longer used. This is likely a
  /// device-specific function.
  ///
  VTKM_CONT void Reset(const std::shared_ptr<vtkm::UInt8> buffer,
                       vtkm::BufferSizeType numberOfBytes,
                       vtkm::cont::DeviceAdapterId device);

  /// \brief Resets the `Buffer` to the memory allocated at the given pointer.
  ///
  /// The `Buffer` is initialized to a state that contains the given `buffer` of data. The memory
  /// is assumed to be deleted with the standard `delete[]` keyword.
  ///
  /// Note that the size passed in is the number of bytes, not the number of values.
  ///
  template <typename T>
  VTKM_CONT void Reset(T* buffer, vtkm::BufferSizeType numberOfBytes)
  {
    this->Reset(buffer, numberOfBytes, std::default_delete<T[]>{});
  }

  /// \brief Resets the `Buffer` to the memory allocated at the given pointer.
  ///
  /// The `Buffer` is initialized to a state that contains the given `buffer` of data. The
  /// `deleter` is an object with an `operator()(void*)` that will properly delete the provided
  /// buffer.
  ///
  /// Note that the size passed in is the number of bytes, not the number of values.
  ///
  template <typename T, typename Deleter>
  VTKM_CONT void Reset(T* buffer, vtkm::BufferSizeType numberOfBytes, Deleter deleter)
  {
    std::shared_ptr<vtkm::UInt8> sharedP(reinterpret_cast<vtkm::UInt8*>(buffer), deleter);
    this->Reset(sharedP, numberOfBytes);
  }

  /// \brief Resets the `Buffer` to the memory allocated at the given pointer on a device.
  ///
  /// The `Buffer` is initialized to a state that contains the given `buffer` of data. The
  /// `deleter` is an object with an `operator()(void*)` that will properly delete the provided
  /// buffer. This is likely a device-specific function.
  ///
  /// Note that the size passed in is the number of bytes, not the number of values.
  ///
  template <typename T, typename Deleter>
  VTKM_CONT void Reset(T* buffer,
                       vtkm::BufferSizeType numberOfBytes,
                       Deleter deleter,
                       vtkm::cont::DeviceAdapterId device)
  {
    std::shared_ptr<vtkm::UInt8> sharedP(reinterpret_cast<vtkm::UInt8*>(buffer), deleter);
    this->Reset(sharedP, numberOfBytes, device);
  }
};
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

template <vtkm::IdComponent N>
struct Serialization<vtkm::Vec<vtkm::cont::internal::Buffer, N>>
{
  static VTKM_CONT void save(BinaryBuffer& bb,
                             const vtkm::Vec<vtkm::cont::internal::Buffer, N>& obj)
  {
    for (vtkm::IdComponent index = 0; index < N; ++index)
    {
      vtkmdiy::save(bb, obj[index]);
    }
  }

  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::Vec<vtkm::cont::internal::Buffer, N>& obj)
  {
    for (vtkm::IdComponent index = 0; index < N; ++index)
    {
      vtkmdiy::load(bb, obj[index]);
    }
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_vtkm_cont_internal_Buffer_h
