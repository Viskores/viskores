//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_TransferInfo_h
#define vtk_m_cont_internal_TransferInfo_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/internal/ArrayPortalVirtual.h>

#include <memory>

namespace vtkm
{

namespace internal
{
class PortalVirtualBase;
}

namespace cont
{
namespace internal
{

struct VTKM_CONT_EXPORT TransferInfoArray
{
  bool valid(vtkm::cont::DeviceAdapterId tagValue) const noexcept;

  void updateHost(std::unique_ptr<vtkm::internal::PortalVirtualBase>&& host) noexcept;
  void updateDevice(
    vtkm::cont::DeviceAdapterId id,
    std::unique_ptr<vtkm::internal::PortalVirtualBase>&& host_copy, //NOT the same as host version
    const vtkm::internal::PortalVirtualBase* device,
    const std::shared_ptr<void>& state) noexcept;
  void releaseDevice();
  void releaseAll();

  const vtkm::internal::PortalVirtualBase* hostPtr() const noexcept { return this->Host.get(); }
  const vtkm::internal::PortalVirtualBase* devicePtr() const noexcept { return this->Device; }
  vtkm::cont::DeviceAdapterId deviceId() const noexcept { return this->DeviceId; }

  std::shared_ptr<void>& state() noexcept { return this->DeviceTransferState; }

private:
  vtkm::cont::DeviceAdapterId DeviceId = vtkm::cont::DeviceAdapterTagUndefined{};
  std::unique_ptr<vtkm::internal::PortalVirtualBase> Host = nullptr;
  std::unique_ptr<vtkm::internal::PortalVirtualBase> HostCopyOfDevice = nullptr;
  const vtkm::internal::PortalVirtualBase* Device = nullptr;
  std::shared_ptr<void> DeviceTransferState = nullptr;
};
}
}
}

#endif
