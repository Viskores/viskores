//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_StorageDeprecated_h
#define vtk_m_cont_internal_StorageDeprecated_h

#include <vtkm/cont/ErrorBadDevice.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/Storage.h>
#include <vtkm/cont/Token.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/cont/internal/Buffer.h>

#include <vtkm/internal/ArrayPortalHelpers.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

template <typename T, typename S>
class ArrayHandleDeprecated;

namespace detail
{

struct TryPrepareInput
{
  template <typename Device, typename ArrayType>
  VTKM_CONT bool operator()(Device device,
                            ArrayType&& array,
                            vtkm::cont::Token& token,
                            typename ArrayType::ReadPortalType& portal,
                            bool& created) const
  {
    if (!created)
    {
      portal = array.PrepareForInput(device, token);
      created = true;
    }
    return true;
  }
};

struct TryPrepareInPlace
{
  template <typename Device, typename ArrayType>
  VTKM_CONT bool operator()(Device device,
                            ArrayType&& array,
                            vtkm::cont::Token& token,
                            typename ArrayType::WritePortalType& portal,
                            bool& created) const
  {
    if (!created)
    {
      portal = array.PrepareForInPlace(device, token);
      created = true;
    }
    return true;
  }
};

template <typename StorageType>
struct StorageTemplateParams;

template <typename T, typename S>
struct StorageTemplateParams<vtkm::cont::internal::Storage<T, S>>
{
  using ValueType = T;
  using StorageTag = S;
};

} // namespace detail

/// \brief `Storage` handler for `ArrayHandle` types still using old `ArrayHandle` style.
///
/// A recent change to `ArrayHandle` moved from using the `ArrayTransfer` method for
/// moving data from control to execution environments to using `Buffer` objects. One
/// feature of the `Buffer` objects is that if you have a new style `ArrayHandle` that
/// deprecates other `ArrayHandle`s, they both have to use `Buffer`.
///
/// All old-style `ArrayHandle`s that still use `ArrayTransfer` should have a
/// `VTKM_STORAGE_OLD_STYLE;` declaration at the bottom of the `Storage` class.
///
template <typename StorageType, typename ReadPortalType, typename WritePortalType>
class StorageDeprecated
{
  using T = typename detail::StorageTemplateParams<StorageType>::ValueType;
  using StorageTag = typename detail::StorageTemplateParams<StorageType>::StorageTag;

  using ArrayType = vtkm::cont::internal::ArrayHandleDeprecated<T, StorageTag>;

  VTKM_CONT static ArrayType GetArray(const vtkm::cont::internal::Buffer* buffers)
  {
    return buffers[0].GetMetaData<ArrayType>();
  }

public:
  VTKM_CONT constexpr static vtkm::IdComponent GetNumberOfBuffers() { return 1; }

  VTKM_CONT static vtkm::Id GetNumberOfValues(const vtkm::cont::internal::Buffer* buffers)
  {
    return GetArray(buffers).GetNumberOfValues();
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      vtkm::cont::internal::Buffer* buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token)
  {
    switch (preserve)
    {
      case vtkm::CopyFlag::Off:
        GetArray(buffers).Allocate(numValues, token);
        break;
      case vtkm::CopyFlag::On:
        GetArray(buffers).Shrink(numValues, token);
        break;
    }
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(const vtkm::cont::internal::Buffer* buffers,
                                                   vtkm::cont::DeviceAdapterId device,
                                                   vtkm::cont::Token& token)
  {
    if (device == vtkm::cont::DeviceAdapterTagUndefined{})
    {
      return GetArray(buffers).ReadPortal();
    }
    else
    {
      ReadPortalType portal;
      bool created = false;
      vtkm::cont::TryExecuteOnDevice(
        device, detail::TryPrepareInput{}, GetArray(buffers), token, portal, created);
      if (!created)
      {
        throw vtkm::cont::ErrorBadDevice("Failed to create input portal for device " +
                                         device.GetName());
      }
      return portal;
    }
  }

private:
  VTKM_CONT static WritePortalType CreateWritePortalImpl(vtkm::cont::internal::Buffer* buffers,
                                                         vtkm::cont::DeviceAdapterId device,
                                                         vtkm::cont::Token& token,
                                                         std::true_type)
  {
    if (device == vtkm::cont::DeviceAdapterTagUndefined{})
    {
      return GetArray(buffers).WritePortal();
    }
    else
    {
      WritePortalType portal;
      bool created = false;
      vtkm::cont::TryExecuteOnDevice(
        device, detail::TryPrepareInPlace{}, GetArray(buffers), token, portal, created);
      if (!created)
      {
        throw vtkm::cont::ErrorBadDevice("Failed to create in place portal for device " +
                                         device.GetName());
      }
      return portal;
    }
  }

  VTKM_CONT static WritePortalType CreateWritePortalImpl(vtkm::cont::internal::Buffer*,
                                                         vtkm::cont::DeviceAdapterId,
                                                         vtkm::cont::Token&,
                                                         std::false_type)
  {
    throw vtkm::cont::ErrorBadType("Attempted to get a writable portal to a read-only array.");
  }

  using SupportsWrite = vtkm::internal::PortalSupportsSets<WritePortalType>;

public:
  VTKM_CONT static WritePortalType CreateWritePortal(vtkm::cont::internal::Buffer* buffers,
                                                     vtkm::cont::DeviceAdapterId device,
                                                     vtkm::cont::Token& token)
  {
    return CreateWritePortalImpl(buffers, device, token, SupportsWrite{});
  }
};

#define VTKM_STORAGE_OLD_STYLE                                                             \
public:                                                                                    \
  using HasOldBridge = std::true_type;                                                     \
  using ReadPortalType = PortalConstType;                                                  \
  using WritePortalType = PortalType;                                                      \
                                                                                           \
private:                                                                                   \
  using StorageDeprecated =                                                                \
    vtkm::cont::internal::StorageDeprecated<Storage, ReadPortalType, WritePortalType>;     \
                                                                                           \
public:                                                                                    \
  VTKM_CONT static vtkm::Id GetNumberOfValues(const vtkm::cont::internal::Buffer* buffers) \
  {                                                                                        \
    return StorageDeprecated::GetNumberOfValues(buffers);                                  \
  }                                                                                        \
  static constexpr auto& GetNumberOfBuffers = StorageDeprecated::GetNumberOfBuffers;       \
  static constexpr auto& ResizeBuffers = StorageDeprecated::ResizeBuffers;                 \
  static constexpr auto& CreateReadPortal = StorageDeprecated::CreateReadPortal;           \
  static constexpr auto& CreateWritePortal = StorageDeprecated::CreateWritePortal

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_StorageDeprecated_h
