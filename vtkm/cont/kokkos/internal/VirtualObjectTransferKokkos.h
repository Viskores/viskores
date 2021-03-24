//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_kokkos_internal_VirtualObjectTransferKokkos_h
#define vtk_m_cont_kokkos_internal_VirtualObjectTransferKokkos_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/internal/VirtualObjectTransfer.h>

#include <vtkm/cont/kokkos/internal/DeviceAdapterTagKokkos.h>
#include <vtkm/cont/kokkos/internal/KokkosAlloc.h>
#include <vtkm/cont/kokkos/internal/KokkosTypes.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename VirtualDerivedType>
struct VirtualObjectTransfer<VirtualDerivedType, vtkm::cont::DeviceAdapterTagKokkos>
{
  VTKM_CONT VirtualObjectTransfer(const VirtualDerivedType* virtualObject)
    : ControlObject(virtualObject)
    , ExecutionObject(nullptr)
  {
  }

  VTKM_CONT ~VirtualObjectTransfer() { this->ReleaseResources(); }

  VirtualObjectTransfer(const VirtualObjectTransfer&) = delete;
  void operator=(const VirtualObjectTransfer&) = delete;

  VTKM_CONT const VirtualDerivedType* PrepareForExecution(bool updateData)
  {
    if (this->ExecutionObject == nullptr || updateData)
    {
      // deviceTarget will hold a byte copy of the host object on the device. The virtual table
      // will be wrong.
      vtkm::cont::kokkos::internal::KokkosViewConstCont<vtkm::UInt8> hbuffer(
        reinterpret_cast<const vtkm::UInt8*>(this->ControlObject), sizeof(VirtualDerivedType));

      auto deviceTarget = static_cast<VirtualDerivedType*>(
        vtkm::cont::kokkos::internal::Allocate(sizeof(VirtualDerivedType)));
      vtkm::cont::kokkos::internal::KokkosViewExec<vtkm::UInt8> dbuffer(
        reinterpret_cast<vtkm::UInt8*>(deviceTarget), sizeof(VirtualDerivedType));
      Kokkos::deep_copy(
        vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), dbuffer, hbuffer);

      if (this->ExecutionObject == nullptr)
      {
        // Allocate memory for the object that will eventually be a correct copy on the device.
        auto executionObjectPtr = this->ExecutionObject = static_cast<VirtualDerivedType*>(
          vtkm::cont::kokkos::internal::Allocate(sizeof(VirtualDerivedType)));
        // Initialize the device object
        Kokkos::RangePolicy<vtkm::cont::kokkos::internal::ExecutionSpace> policy(
          vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), 0, 1);
        Kokkos::parallel_for(
          "ConstructVirtualObject", policy, KOKKOS_LAMBDA(const int&) {
            new (executionObjectPtr) VirtualDerivedType(*deviceTarget);
          });
      }
      else if (updateData)
      {
        auto executionObjectPtr = this->ExecutionObject;
        // Initialize the device object
        Kokkos::RangePolicy<vtkm::cont::kokkos::internal::ExecutionSpace> policy(
          vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), 0, 1);
        Kokkos::parallel_for(
          "UpdateVirtualObject", policy, KOKKOS_LAMBDA(const int&) {
            *executionObjectPtr = *deviceTarget;
          });
      }

      vtkm::cont::kokkos::internal::Free(deviceTarget);
    }

    return this->ExecutionObject;
  }

  VTKM_CONT void ReleaseResources()
  {
    if (this->ExecutionObject != nullptr)
    {
      auto executionObjectPtr = this->ExecutionObject;
      this->ExecutionObject = nullptr;

      Kokkos::RangePolicy<vtkm::cont::kokkos::internal::ExecutionSpace> policy(
        vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), 0, 1);
      Kokkos::parallel_for(
        "DeleteVirtualObject", policy, KOKKOS_LAMBDA(const int&) {
          executionObjectPtr->~VirtualDerivedType();
        });
      vtkm::cont::kokkos::internal::Free(executionObjectPtr);
    }
  }

private:
  const VirtualDerivedType* ControlObject;
  VirtualDerivedType* ExecutionObject;
};
}
}
} // vtkm::cont::internal

#endif // vtk_m_cont_kokkos_internal_VirtualObjectTransferKokkos_h
