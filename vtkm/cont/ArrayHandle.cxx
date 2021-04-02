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

namespace
{

struct DeviceCheckFunctor
{
  vtkm::cont::DeviceAdapterId FoundDevice = vtkm::cont::DeviceAdapterTagUndefined{};

  VTKM_CONT void operator()(vtkm::cont::DeviceAdapterId device,
                            const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    if (this->FoundDevice == vtkm::cont::DeviceAdapterTagUndefined{})
    {
      if (vtkm::cont::detail::ArrayHandleIsOnDevice(buffers, device))
      {
        this->FoundDevice = device;
      }
    }
  }
};

} // anonymous namespace

namespace vtkm
{
namespace cont
{
namespace detail
{

VTKM_CONT vtkm::cont::DeviceAdapterId ArrayHandleGetDeviceAdapterId(
  const std::vector<vtkm::cont::internal::Buffer>& buffers)
{
  DeviceCheckFunctor functor;

  vtkm::ListForEach(functor, VTKM_DEFAULT_DEVICE_ADAPTER_LIST{}, buffers);

  return functor.FoundDevice;
}
}
}
} // namespace vtkm::cont::detail
