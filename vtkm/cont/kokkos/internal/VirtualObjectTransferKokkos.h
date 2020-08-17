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
#include <vtkm/cont/kokkos/internal/ViewTypes.h>

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
      auto dbuffer = Kokkos::create_mirror_view_and_copy(Kokkos::DefaultExecutionSpace{}, hbuffer);
      auto deviceTarget = reinterpret_cast<const VirtualDerivedType*>(dbuffer.data());

      if (this->ExecutionObject == nullptr)
      {
        // Allocate memory for the object that will eventually be a correct copy on the device.
        auto executionObjectPtr = this->ExecutionObject =
          static_cast<VirtualDerivedType*>(Kokkos::kokkos_malloc(sizeof(VirtualDerivedType)));
        // Initialize the device object
        Kokkos::parallel_for(
          "ConstructVirtualObject", 1, KOKKOS_LAMBDA(const int&) {
            new (executionObjectPtr) VirtualDerivedType(*deviceTarget);
          });
      }
      else if (updateData)
      {
        auto executionObjectPtr = this->ExecutionObject;
        // Initialize the device object
        Kokkos::parallel_for(
          "UpdateVirtualObject", 1, KOKKOS_LAMBDA(const int&) {
            *executionObjectPtr = *deviceTarget;
          });
      }
    }

    return this->ExecutionObject;
  }

  VTKM_CONT void ReleaseResources()
  {
    if (this->ExecutionObject != nullptr)
    {
      auto executionObjectPtr = this->ExecutionObject;
      this->ExecutionObject = nullptr;

      Kokkos::DefaultExecutionSpace execSpace;
      Kokkos::parallel_for(
        "DeleteVirtualObject",
        Kokkos::RangePolicy<Kokkos::DefaultExecutionSpace>(execSpace, 0, 1),
        KOKKOS_LAMBDA(const int&) { executionObjectPtr->~VirtualDerivedType(); });
      execSpace.fence();
      Kokkos::kokkos_free(executionObjectPtr);
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
