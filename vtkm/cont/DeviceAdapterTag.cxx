//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DeviceAdapterTag.h>

#include <vtkm/cont/RuntimeDeviceInformation.h>

namespace vtkm
{
namespace cont
{

DeviceAdapterNameType DeviceAdapterId::GetName() const
{
  vtkm::cont::RuntimeDeviceInformation info;
  return info.GetName(*this);
}

DeviceAdapterId make_DeviceAdapterId(const DeviceAdapterNameType& name)
{
  vtkm::cont::RuntimeDeviceInformation info;
  return info.GetId(name);
}
}
} // end namespace vtkm::cont
