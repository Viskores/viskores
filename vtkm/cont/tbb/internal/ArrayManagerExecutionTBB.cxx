//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/tbb/internal/ArrayManagerExecutionTBB.h>

namespace vtkm {
namespace cont {

#ifdef VTKM_BUILD_PREPARE_FOR_DEVICE
IMPORT_ARRAYHANDLE_DEVICE_ADAPTER(char, DeviceAdapterTagTBB)
IMPORT_ARRAYHANDLE_DEVICE_ADAPTER(vtkm::Int8, DeviceAdapterTagTBB)
IMPORT_ARRAYHANDLE_DEVICE_ADAPTER(vtkm::UInt8, DeviceAdapterTagTBB)
IMPORT_ARRAYHANDLE_DEVICE_ADAPTER(vtkm::Int32, DeviceAdapterTagTBB)
IMPORT_ARRAYHANDLE_DEVICE_ADAPTER(vtkm::Int64, DeviceAdapterTagTBB)
IMPORT_ARRAYHANDLE_DEVICE_ADAPTER(vtkm::Float32, DeviceAdapterTagTBB)
IMPORT_ARRAYHANDLE_DEVICE_ADAPTER(vtkm::Float64, DeviceAdapterTagTBB)
#endif // VTKM_BUILD_PREPARE_FOR_DEVICE

}
} // end vtkm::cont
