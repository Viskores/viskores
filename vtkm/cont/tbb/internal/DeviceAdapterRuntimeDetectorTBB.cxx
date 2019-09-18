//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/tbb/internal/DeviceAdapterRuntimeDetectorTBB.h>
namespace vtkm
{
namespace cont
{
VTKM_CONT bool DeviceAdapterRuntimeDetector<vtkm::cont::DeviceAdapterTagTBB>::Exists() const
{
  return vtkm::cont::DeviceAdapterTagTBB::IsEnabled;
}
}
}
