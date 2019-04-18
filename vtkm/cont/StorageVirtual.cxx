//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define vtk_m_cont_StorageVirtual_cxx
#include <vtkm/cont/StorageVirtual.h>

namespace vtkm
{
namespace cont
{
namespace internal
{
namespace detail
{


//--------------------------------------------------------------------
StorageVirtual::StorageVirtual(const StorageVirtual& src)
  : HostUpToDate(src.HostUpToDate)
  , DeviceUpToDate(src.DeviceUpToDate)
  , DeviceTransferState(src.DeviceTransferState)
{
}

//--------------------------------------------------------------------
StorageVirtual::StorageVirtual(StorageVirtual&& src) noexcept
  : HostUpToDate(src.HostUpToDate),
    DeviceUpToDate(src.DeviceUpToDate),
    DeviceTransferState(std::move(src.DeviceTransferState))
{
}

//--------------------------------------------------------------------
StorageVirtual& StorageVirtual::operator=(const StorageVirtual& src)
{
  this->HostUpToDate = src.HostUpToDate;
  this->DeviceUpToDate = src.DeviceUpToDate;
  this->DeviceTransferState = src.DeviceTransferState;
  return *this;
}

//--------------------------------------------------------------------
StorageVirtual& StorageVirtual::operator=(StorageVirtual&& src) noexcept
{
  this->HostUpToDate = src.HostUpToDate;
  this->DeviceUpToDate = src.DeviceUpToDate;
  this->DeviceTransferState = std::move(src.DeviceTransferState);
  return *this;
}

//--------------------------------------------------------------------
StorageVirtual::~StorageVirtual()
{
}

//--------------------------------------------------------------------
void StorageVirtual::DropExecutionPortal()
{
  this->DeviceTransferState->releaseDevice();
  this->DeviceUpToDate = false;
}

//--------------------------------------------------------------------
void StorageVirtual::DropAllPortals()
{
  this->DeviceTransferState->releaseAll();
  this->HostUpToDate = false;
  this->DeviceUpToDate = false;
}

//--------------------------------------------------------------------
std::unique_ptr<StorageVirtual> StorageVirtual::NewInstance() const
{
  return this->MakeNewInstance();
}

//--------------------------------------------------------------------
const vtkm::internal::PortalVirtualBase* StorageVirtual::PrepareForInput(
  vtkm::cont::DeviceAdapterId devId) const
{
  if (devId == vtkm::cont::DeviceAdapterTagUndefined())
  {
    throw vtkm::cont::ErrorBadValue("device should not be VTKM_DEVICE_ADAPTER_UNDEFINED");
  }

  const bool needsUpload = !(this->DeviceTransferState->valid(devId) && this->DeviceUpToDate);

  if (needsUpload)
  { //Either transfer state is pointing to another device, or has
    //had the execution resources released. Either way we
    //need to re-transfer the execution information
    auto* payload = this->DeviceTransferState.get();
    this->TransferPortalForInput(*payload, devId);
    this->DeviceUpToDate = true;
  }
  return this->DeviceTransferState->devicePtr();
}

//--------------------------------------------------------------------
const vtkm::internal::PortalVirtualBase* StorageVirtual::PrepareForOutput(
  vtkm::Id numberOfValues,
  vtkm::cont::DeviceAdapterId devId)
{
  if (devId == vtkm::cont::DeviceAdapterTagUndefined())
  {
    throw vtkm::cont::ErrorBadValue("device should not be VTKM_DEVICE_ADAPTER_UNDEFINED");
  }

  const bool needsUpload = !(this->DeviceTransferState->valid(devId) && this->DeviceUpToDate);
  if (needsUpload)
  {
    this->TransferPortalForOutput(
      *(this->DeviceTransferState), OutputMode::WRITE, numberOfValues, devId);
    this->HostUpToDate = false;
    this->DeviceUpToDate = true;
  }
  return this->DeviceTransferState->devicePtr();
}

//--------------------------------------------------------------------
const vtkm::internal::PortalVirtualBase* StorageVirtual::PrepareForInPlace(
  vtkm::cont::DeviceAdapterId devId)
{
  if (devId == vtkm::cont::DeviceAdapterTagUndefined())
  {
    throw vtkm::cont::ErrorBadValue("device should not be VTKM_DEVICE_ADAPTER_UNDEFINED");
  }

  const bool needsUpload = !(this->DeviceTransferState->valid(devId) && this->DeviceUpToDate);
  if (needsUpload)
  {
    vtkm::Id numberOfValues = this->GetNumberOfValues();
    this->TransferPortalForOutput(
      *(this->DeviceTransferState), OutputMode::READ_WRITE, numberOfValues, devId);
    this->HostUpToDate = false;
    this->DeviceUpToDate = true;
  }
  return this->DeviceTransferState->devicePtr();
}

//--------------------------------------------------------------------
const vtkm::internal::PortalVirtualBase* StorageVirtual::GetPortalControl()
{
  if (!this->HostUpToDate)
  {
    //we need to prepare for input and grab the host ptr
    auto* payload = this->DeviceTransferState.get();
    this->ControlPortalForOutput(*payload);
  }

  this->DeviceUpToDate = false;
  this->HostUpToDate = true;
  return this->DeviceTransferState->hostPtr();
}

//--------------------------------------------------------------------
const vtkm::internal::PortalVirtualBase* StorageVirtual::GetPortalConstControl() const
{
  if (!this->HostUpToDate)
  {
    //we need to prepare for input and grab the host ptr
    vtkm::cont::internal::TransferInfoArray* payload = this->DeviceTransferState.get();
    this->ControlPortalForInput(*payload);
  }
  this->HostUpToDate = true;
  return this->DeviceTransferState->hostPtr();
}

//--------------------------------------------------------------------
DeviceAdapterId StorageVirtual::GetDeviceAdapterId() const noexcept
{
  return this->DeviceTransferState->deviceId();
}

//--------------------------------------------------------------------
void StorageVirtual::ControlPortalForOutput(vtkm::cont::internal::TransferInfoArray&)
{
  throw vtkm::cont::ErrorBadValue(
    "StorageTagVirtual by default doesn't support control side writes.");
}

//--------------------------------------------------------------------
void StorageVirtual::TransferPortalForOutput(vtkm::cont::internal::TransferInfoArray&,
                                             OutputMode,
                                             vtkm::Id,
                                             vtkm::cont::DeviceAdapterId)
{
  throw vtkm::cont::ErrorBadValue("StorageTagVirtual by default doesn't support exec side writes.");
}

#define VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(T)                                                \
  template class VTKM_CONT_EXPORT ArrayTransferVirtual<T>;                                         \
  template class VTKM_CONT_EXPORT ArrayTransferVirtual<vtkm::Vec<T, 2>>;                           \
  template class VTKM_CONT_EXPORT ArrayTransferVirtual<vtkm::Vec<T, 3>>;                           \
  template class VTKM_CONT_EXPORT ArrayTransferVirtual<vtkm::Vec<T, 4>>

VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(char);
VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(vtkm::Int8);
VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(vtkm::UInt8);
VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(vtkm::Int16);
VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(vtkm::UInt16);
VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(vtkm::Int32);
VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(vtkm::UInt32);
VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(vtkm::Int64);
VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(vtkm::UInt64);
VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(vtkm::Float32);
VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(vtkm::Float64);

#undef VTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE

#define VTK_M_STORAGE_VIRTUAL_INSTANTIATE(T)                                                       \
  template class VTKM_CONT_EXPORT StorageVirtualImpl<T, VTKM_DEFAULT_STORAGE_TAG>;                 \
  template class VTKM_CONT_EXPORT StorageVirtualImpl<vtkm::Vec<T, 2>, VTKM_DEFAULT_STORAGE_TAG>;   \
  template class VTKM_CONT_EXPORT StorageVirtualImpl<vtkm::Vec<T, 3>, VTKM_DEFAULT_STORAGE_TAG>;   \
  template class VTKM_CONT_EXPORT StorageVirtualImpl<vtkm::Vec<T, 4>, VTKM_DEFAULT_STORAGE_TAG>

VTK_M_STORAGE_VIRTUAL_INSTANTIATE(char);
VTK_M_STORAGE_VIRTUAL_INSTANTIATE(vtkm::Int8);
VTK_M_STORAGE_VIRTUAL_INSTANTIATE(vtkm::UInt8);
VTK_M_STORAGE_VIRTUAL_INSTANTIATE(vtkm::Int16);
VTK_M_STORAGE_VIRTUAL_INSTANTIATE(vtkm::UInt16);
VTK_M_STORAGE_VIRTUAL_INSTANTIATE(vtkm::Int32);
VTK_M_STORAGE_VIRTUAL_INSTANTIATE(vtkm::UInt32);
VTK_M_STORAGE_VIRTUAL_INSTANTIATE(vtkm::Int64);
VTK_M_STORAGE_VIRTUAL_INSTANTIATE(vtkm::UInt64);
VTK_M_STORAGE_VIRTUAL_INSTANTIATE(vtkm::Float32);
VTK_M_STORAGE_VIRTUAL_INSTANTIATE(vtkm::Float64);

#undef VTK_M_STORAGE_VIRTUAL_INSTANTIATE
}
}
}
} // namespace vtkm::cont::internal::detail
