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
#include <vtkm/cont/RuntimeDeviceInformation.h>

#include <vtkm/ListTag.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterListTag.h>

//Bring in each device adapters runtime class
#include <vtkm/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>
#include <vtkm/cont/internal/DeviceAdapterError.h>
#include <vtkm/cont/openmp/internal/DeviceAdapterRuntimeDetectorOpenMP.h>
#include <vtkm/cont/serial/internal/DeviceAdapterRuntimeDetectorSerial.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterRuntimeDetectorTBB.h>

namespace vtkm
{
namespace cont
{
namespace detail
{
struct RuntimeDeviceInformationFunctor
{
  bool Exists = false;
  template <typename DeviceAdapter>
  VTKM_CONT void operator()(DeviceAdapter, DeviceAdapterId device)
  {
    if (DeviceAdapter() == device)
    {
      this->Exists = vtkm::cont::DeviceAdapterRuntimeDetector<DeviceAdapter>().Exists();
    }
  }
};
}

VTKM_CONT
bool RuntimeDeviceInformation::Exists(DeviceAdapterId id) const
{
  detail::RuntimeDeviceInformationFunctor functor;
  vtkm::ListForEach(functor, VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG(), id);
  return functor.Exists;
}
}
} // namespace vtkm::cont
