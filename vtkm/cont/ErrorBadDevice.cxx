//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/ErrorBadDevice.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

#include <string>

namespace vtkm
{
namespace cont
{

void throwFailedRuntimeDeviceTransfer(const std::string& className,
                                      vtkm::cont::DeviceAdapterId deviceId)
{ //Should we support typeid() instead of className?
  const std::string msg = "VTK-m was unable to transfer " + className + " to DeviceAdapter[id=" +
    std::to_string(deviceId.GetValue()) + ", name=" + deviceId.GetName() +
    "]. This is generally caused by asking for execution on a DeviceAdapter that "
    "isn't compiled into VTK-m. In the case of CUDA it can also be caused by accidentally "
    "compiling source files as C++ files instead of CUDA.";
  throw vtkm::cont::ErrorBadDevice(msg);
}
}
}
