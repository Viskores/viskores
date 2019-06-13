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

#include <vtkm/cont/testing/TestingColorTable.h>

int UnitTestSerialColorTable(int argc, char* argv[])
{
  //TestingColorTable forces the device
  return vtkm::cont::testing::TestingColorTable<vtkm::cont::DeviceAdapterTagSerial>::Run(argc,
                                                                                         argv);
}
