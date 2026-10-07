//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
