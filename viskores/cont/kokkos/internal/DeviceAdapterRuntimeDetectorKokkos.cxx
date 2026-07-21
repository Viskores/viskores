//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
