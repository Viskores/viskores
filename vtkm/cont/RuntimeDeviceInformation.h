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
#ifndef vtk_m_cont_RuntimeDeviceInformation_h
#define vtk_m_cont_RuntimeDeviceInformation_h

#include <vtkm/cont/DeviceAdapter.h>

//Bring in each device adapters runtime class
#include <vtkm/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>
#include <vtkm/cont/internal/DeviceAdapterError.h>
#include <vtkm/cont/serial/internal/DeviceAdapterRuntimeDetectorSerial.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterRuntimeDetectorTBB.h>

namespace vtkm
{
namespace cont
{

/// A class that can be used to determine if a given device adapter
/// is supported on the current machine at runtime. This is very important
/// for device adapters that a physical hardware requirements such as a GPU
/// or a Accelerator Card.
///
///
template <class Device>
class RuntimeDeviceInformation
{
public:
  VTKM_CONT
  RuntimeDeviceInformation()
    : RuntimeImplementation()
  {
  }

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  VTKM_CONT
  bool Exists() const { return this->RuntimeImplementation.Exists(); }

private:
  vtkm::cont::DeviceAdapterRuntimeDetector<Device> RuntimeImplementation;
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_RuntimeDeviceInformation_h
