//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace cont
{
namespace detail
{

VTKM_CONT void ArrayHandleReleaseResourcesExecution(
  const std::vector<vtkm::cont::internal::Buffer>& buffers)
{
  vtkm::cont::Token token;

  for (auto&& buf : buffers)
  {
    buf.ReleaseDeviceResources();
  }
}

VTKM_CONT bool ArrayHandleIsOnDevice(const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                     vtkm::cont::DeviceAdapterId device)
{
  for (auto&& buf : buffers)
  {
    if (!buf.IsAllocatedOnDevice(device))
    {
      return false;
    }
  }
  return true;
}
}
}
} // namespace vtkm::cont::detail
