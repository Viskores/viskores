//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Error.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

//------------------------------------------------------------------------------
// This test ensures that exceptions thrown internally by the vtkm_cont library
// can be correctly caught across library boundaries.
int UnitTestExceptions(int argc, char* argv[])
{
  vtkm::cont::Initialize(argc, argv);
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();

  try
  {
    // This throws a ErrorBadValue from RuntimeDeviceTracker::CheckDevice,
    // which is compiled into the vtkm_cont library:
    tracker.ResetDevice(vtkm::cont::DeviceAdapterTagUndefined());
  }
  catch (vtkm::cont::ErrorBadValue&)
  {
    return EXIT_SUCCESS;
  }

  std::cerr << "Did not catch expected ErrorBadValue exception.\n";
  return EXIT_FAILURE;
}
