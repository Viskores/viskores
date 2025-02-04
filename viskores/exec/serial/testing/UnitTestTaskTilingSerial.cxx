//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/testing/Testing.h>

#include <viskores/cont/serial/DeviceAdapterSerial.h>
#include <viskores/exec/testing/TestingTaskTiling.h>

int UnitTestTaskTilingSerial(int argc, char* argv[])
{

  return viskores::testing::Testing::Run(
    viskores::exec::internal::testing::TestTaskTiling<viskores::cont::DeviceAdapterTagSerial>, argc, argv);
}
