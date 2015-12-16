//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_RuntimeDeviceInformation_h
#define vtk_m_cont_RuntimeDeviceInformation_h

#include <vtkm/cont/DeviceAdapter.h>

namespace vtkm {
namespace cont {

/// A class that can be used to determine if a given device adapter
/// is supported on the current machine at runtime. This is very important
/// for device adapters that a physical hardware requirements such as a GPU
/// or a Accelerator Card.
///
///
template<class Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class RuntimeDeviceInformation
{
public:
  VTKM_CONT_EXPORT
  RuntimeDeviceInformation() : RuntimeImplementation() {  }

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  VTKM_CONT_EXPORT
  bool Exists() const
  {
    return this->RuntimeImplementation.Exists();
  }

private:
  vtkm::cont::DeviceAdapterRuntimeDetector<Device> RuntimeImplementation;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_RuntimeDeviceInformation_h
