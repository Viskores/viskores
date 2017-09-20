//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/Error.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/cont/internal/DeviceAdapterError.h>

//------------------------------------------------------------------------------
// This test ensures that exceptions thrown internally by the vtkm_cont library
// can be correctly caught across library boundaries.
int UnitTestExceptions(int, char* [])
{
  vtkm::cont::RuntimeDeviceTracker tracker;

  try
  {
    // This throws a ErrorBadValue from RuntimeDeviceTracker::CheckDevice,
    // which is compiled into the vtkm_cont library:
    tracker.ResetDevice(vtkm::cont::DeviceAdapterTagError());
  }
  catch (vtkm::cont::ErrorBadValue&)
  {
    return EXIT_SUCCESS;
  }

  std::cerr << "Did not catch expected ErrorBadValue exception.\n";
  return EXIT_FAILURE;
}
