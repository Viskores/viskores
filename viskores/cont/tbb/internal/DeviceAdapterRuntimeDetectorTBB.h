//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_cont_tbb_internal_DeviceAdapterRuntimeDetector_h
#define viskores_cont_tbb_internal_DeviceAdapterRuntimeDetector_h

#include <viskores/cont/tbb/internal/DeviceAdapterTagTBB.h>
#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{

template <class DeviceAdapterTag>
class DeviceAdapterRuntimeDetector;

/// Determine if this machine supports Serial backend
///
template <>
class VISKORES_CONT_EXPORT DeviceAdapterRuntimeDetector<viskores::cont::DeviceAdapterTagTBB>
{
public:
  /// Returns true if the given device adapter is supported on the current
  /// machine.
  VISKORES_CONT bool Exists() const;
};
}
}

#endif
