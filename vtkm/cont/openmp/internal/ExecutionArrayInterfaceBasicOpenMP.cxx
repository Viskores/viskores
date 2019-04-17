//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/openmp/internal/ExecutionArrayInterfaceBasicOpenMP.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

DeviceAdapterId ExecutionArrayInterfaceBasic<DeviceAdapterTagOpenMP>::GetDeviceId() const
{
  return DeviceAdapterTagOpenMP{};
}

} // namespace internal
}
} // namespace vtkm::cont
