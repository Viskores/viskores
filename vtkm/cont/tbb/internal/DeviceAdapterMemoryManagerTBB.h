//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_tbb_internal_DeviceAdapterMemoryManagerTBB_h
#define vtk_m_cont_tbb_internal_DeviceAdapterMemoryManagerTBB_h

#include <vtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>

#include <vtkm/cont/internal/DeviceAdapterMemoryManagerShared.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
class DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagTBB>
  : public vtkm::cont::internal::DeviceAdapterMemoryManagerShared
{
  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const override
  {
    return vtkm::cont::DeviceAdapterTagTBB{};
  }
};
}
}
}

#endif //vtk_m_cont_tbb_internal_DeviceAdapterMemoryManagerTBB_h
