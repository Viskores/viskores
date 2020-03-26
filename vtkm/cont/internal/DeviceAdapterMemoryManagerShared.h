//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_DeviceAdapterMemoryManagerShared_h
#define vtk_m_cont_internal_DeviceAdapterMemoryManagerShared_h

#include <vtkm/cont/internal/DeviceAdapterMemoryManager.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// \brief An implementation of DeviceAdapterMemoryManager for devices that share memory with the
/// host.
///
/// Device adapters that have shared memory with the host can implement their
/// `DeviceAdapterMemoryManager` as a simple subclass of this.
///
class VTKM_CONT_EXPORT DeviceAdapterMemoryManagerShared : public DeviceAdapterMemoryManagerBase
{
public:
  VTKM_CONT std::shared_ptr<BufferInfo> Allocate(vtkm::BufferSizeType size) override;

  VTKM_CONT std::shared_ptr<BufferInfo> ManageArray(std::shared_ptr<vtkm::UInt8> buffer,
                                                    vtkm::BufferSizeType size) override;

  VTKM_CONT void Reallocate(std::shared_ptr<vtkm::cont::internal::BufferInfo> buffer,
                            vtkm::BufferSizeType newSize) override;

  VTKM_CONT std::shared_ptr<vtkm::cont::internal::BufferInfo> CopyHostToDevice(
    std::shared_ptr<vtkm::cont::internal::BufferInfoHost> src) override;

  VTKM_CONT std::shared_ptr<vtkm::cont::internal::BufferInfoHost> CopyDeviceToHost(
    std::shared_ptr<vtkm::cont::internal::BufferInfo> src) override;

  VTKM_CONT std::shared_ptr<vtkm::cont::internal::BufferInfo> CopyDeviceToDevice(
    std::shared_ptr<vtkm::cont::internal::BufferInfo> src) override;
};
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_DeviceAdapterMemoryManagerShared_h
