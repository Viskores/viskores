//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/kokkos/internal/DeviceAdapterRuntimeDetectorKokkos.h>

namespace viskores
{
namespace cont
{
VISKORES_CONT bool DeviceAdapterRuntimeDetector<viskores::cont::DeviceAdapterTagKokkos>::Exists()
  const
{
  return viskores::cont::DeviceAdapterTagKokkos::IsEnabled;
}
}
}
