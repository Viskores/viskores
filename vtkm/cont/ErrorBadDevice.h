//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ErrorBadDevice_h
#define vtk_m_cont_ErrorBadDevice_h

#include <vtkm/cont/Error.h>

#include <vtkm/cont/internal/DeviceAdapterTag.h>
#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{


VTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown when VTK-m performs an operation that is not supported
/// on the current device.
///
class VTKM_ALWAYS_EXPORT ErrorBadDevice : public Error
{
public:
  ErrorBadDevice(const std::string& message)
    : Error(message)
  {
  }
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END

/// Throws an ErrorBadeDevice exception with the following message:
/// "VTK-m was unable to transfer \c className to DeviceAdapter[id=\c device].
///  This is generally caused by asking for execution on a DeviceAdapter that
///  isn't compiled into VTK-m. In the case of CUDA it can also be caused by accidentally
///  compiling source files as C++ files instead of CUDA."
//
VTKM_CONT_EXPORT void throwFailedRuntimeDeviceTransfer(const std::string& className,
                                                       vtkm::cont::DeviceAdapterId device);
}
} // namespace vtkm::cont

#endif // vtk_m_cont_ErrorBadDevice_h
