//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_serial_internal_DeviceAdapterRuntimeDetector_h
#define viskores_cont_serial_internal_DeviceAdapterRuntimeDetector_h

#include <viskores/cont/serial/internal/DeviceAdapterTagSerial.h>
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
class VISKORES_CONT_EXPORT DeviceAdapterRuntimeDetector<viskores::cont::DeviceAdapterTagSerial>
{
public:
  /// Returns true if the given device adapter is supported on the current
  /// machine.
  VISKORES_CONT bool Exists() const;
};
}
}

#endif
