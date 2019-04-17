//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/TestingComputeRange.h>

int UnitTestSerialComputeRange(int argc, char* argv[])
{
  //TestingComputeRange forces the device
  return vtkm::cont::testing::TestingComputeRange<vtkm::cont::DeviceAdapterTagSerial>::Run(argc,
                                                                                           argv);
}
