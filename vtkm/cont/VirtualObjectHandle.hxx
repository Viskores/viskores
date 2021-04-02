//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_VirtualObjectHandle_hxx
#define vtk_m_cont_VirtualObjectHandle_hxx

#include <vtkm/cont/VirtualObjectHandle.h>

#include <vtkm/cont/DeviceAdapterList.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/internal/DeviceAdapterListHelpers.h>

#include <vtkm/cont/internal/VirtualObjectTransfer.h>
#include <vtkm/cont/internal/VirtualObjectTransfer.hxx>

namespace vtkm
{
namespace cont
{

template <typename VirtualBaseType>
VirtualObjectHandle<VirtualBaseType>::VirtualObjectHandle()
  : Internals(std::make_shared<internal::TransferState>())
{
}

template <typename VirtualBaseType>
template <typename VirtualDerivedType, typename DeviceAdapterList>
VirtualObjectHandle<VirtualBaseType>::VirtualObjectHandle(VirtualDerivedType* derived,
                                                          bool acquireOwnership,
                                                          DeviceAdapterList devices)
  : Internals(std::make_shared<internal::TransferState>())
{
  this->Reset(derived, acquireOwnership, devices);
}

template <typename VirtualBaseType>
bool VirtualObjectHandle<VirtualBaseType>::GetValid() const
{
  return this->Internals->HostPtr() != nullptr;
}

template <typename VirtualBaseType>
bool VirtualObjectHandle<VirtualBaseType>::OwnsObject() const
{
  return this->Internals->WillReleaseHostPointer();
}

template <typename VirtualBaseType>
VirtualBaseType* VirtualObjectHandle<VirtualBaseType>::Get()
{
  return static_cast<VirtualBaseType*>(this->Internals->HostPtr());
}

/// Reset the underlying derived type object
template <typename VirtualBaseType>
template <typename VirtualDerivedType, typename DeviceAdapterList>
void VirtualObjectHandle<VirtualBaseType>::Reset(VirtualDerivedType* derived,
                                                 bool acquireOwnership,
                                                 DeviceAdapterList devices)
{
  VTKM_STATIC_ASSERT_MSG((std::is_base_of<VirtualBaseType, VirtualDerivedType>::value),
                         "Tried to bind a type that is not a subclass of the base class.");

  if (acquireOwnership)
  {
    // auto deleter = [](void* p) { delete static_cast<VirtualBaseType*>(p); };
    this->Internals->UpdateHost(derived, nullptr);
  }
  else
  {
    this->Internals->UpdateHost(derived, nullptr);
  }

  if (derived)
  {
    vtkm::cont::internal::ForEachValidDevice(
      devices, internal::CreateTransferInterface(), this->Internals.get(), derived);
  }
}

template <typename VirtualBaseType>
const VirtualBaseType* VirtualObjectHandle<VirtualBaseType>::PrepareForExecution(
  vtkm::cont::DeviceAdapterId deviceId) const
{
  const bool validId = this->Internals->DeviceIdIsValid(deviceId);
  if (!validId)
  { //can't be reached since DeviceIdIsValid will through an exception
    //if deviceId is not valid
    return nullptr;
  }

  return static_cast<const VirtualBaseType*>(this->Internals->PrepareForExecution(deviceId));
}
}
} // vtkm::cont

#endif // vtk_m_cont_VirtualObjectHandle_h
