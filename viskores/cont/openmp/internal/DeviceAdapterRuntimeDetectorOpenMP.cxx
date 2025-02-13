//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/openmp/internal/DeviceAdapterRuntimeDetectorOpenMP.h>

namespace viskores
{
namespace cont
{
VISKORES_CONT bool DeviceAdapterRuntimeDetector<viskores::cont::DeviceAdapterTagOpenMP>::Exists()
  const
{
  return viskores::cont::DeviceAdapterTagOpenMP::IsEnabled;
}
}
}
