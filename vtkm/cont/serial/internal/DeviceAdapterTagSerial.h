//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_serial_internal_DeviceAdapterTagSerial_h
#define vtk_m_cont_serial_internal_DeviceAdapterTagSerial_h

#include <vtkm/cont/DeviceAdapterTag.h>

/// @struct vtkm::cont::DeviceAdapterTagSerial
/// @brief Tag for a device adapter that performs all computation on the
/// same single thread as the control environment.
///
/// This device is useful for debugging. This device is always available. This tag is
/// defined in `vtkm/cont/DeviceAdapterSerial.h`.

VTKM_VALID_DEVICE_ADAPTER(Serial, VTKM_DEVICE_ADAPTER_SERIAL);

#endif //vtk_m_cont_serial_internal_DeviceAdapterTagSerial_h
