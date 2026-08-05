//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_cont_serial_internal_DeviceAdapterTagSerial_h
#define viskores_cont_serial_internal_DeviceAdapterTagSerial_h

#include <viskores/cont/DeviceAdapterTag.h>

/// @struct viskores::cont::DeviceAdapterTagSerial
/// @brief Tag for a device adapter that performs all computation on the
/// same single thread as the control environment.
///
/// This device is useful for debugging. This device is always available. This tag is
/// defined in `viskores/cont/DeviceAdapterSerial.h`.

VISKORES_VALID_DEVICE_ADAPTER(Serial, VISKORES_DEVICE_ADAPTER_SERIAL);

#endif //viskores_cont_serial_internal_DeviceAdapterTagSerial_h
