//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/serial/internal/DeviceAdapterRuntimeDetectorSerial.h>

namespace viskores
{
namespace cont
{
VISKORES_CONT bool DeviceAdapterRuntimeDetector<viskores::cont::DeviceAdapterTagSerial>::Exists()
  const
{
  return viskores::cont::DeviceAdapterTagSerial::IsEnabled;
}
}
}
