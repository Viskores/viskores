//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_ExecutionArrayInterfaceCuda_h
#define vtk_m_cont_cuda_internal_ExecutionArrayInterfaceCuda_h

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>

namespace vtkm
{
namespace cont
{
namespace internal
{
template <>
struct VTKM_CONT_EXPORT ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda> final
  : public ExecutionArrayInterfaceBasicBase
{
  //inherit our parents constructor
  using ExecutionArrayInterfaceBasicBase::ExecutionArrayInterfaceBasicBase;

  VTKM_CONT DeviceAdapterId GetDeviceId() const final;
  VTKM_CONT void Allocate(TypelessExecutionArray& execArray,
                          vtkm::Id numberOfValues,
                          vtkm::UInt64 sizeOfValue) const final;
  VTKM_CONT void Free(TypelessExecutionArray& execArray) const final;
  VTKM_CONT void CopyFromControl(const void* controlPtr,
                                 void* executionPtr,
                                 vtkm::UInt64 numBytes) const final;
  VTKM_CONT void CopyToControl(const void* executionPtr,
                               void* controlPtr,
                               vtkm::UInt64 numBytes) const final;

  VTKM_CONT void UsingForRead(const void* controlPtr,
                              const void* executionPtr,
                              vtkm::UInt64 numBytes) const final;
  VTKM_CONT void UsingForWrite(const void* controlPtr,
                               const void* executionPtr,
                               vtkm::UInt64 numBytes) const final;
  VTKM_CONT void UsingForReadWrite(const void* controlPtr,
                                   const void* executionPtr,
                                   vtkm::UInt64 numBytes) const final;
};
} // namespace internal
}
} // namespace vtkm::cont

#endif //vtk_m_cont_cuda_internal_ExecutionArrayInterfaceCuda_h
