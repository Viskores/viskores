//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/testing/Testing.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/exec/internal/testing/TestingTaskTiling.h>

int UnitTestTaskTilingSerial(int argc, char* argv[])
{

  return vtkm::testing::Testing::Run(
    vtkm::exec::internal::testing::TestTaskTiling<vtkm::cont::DeviceAdapterTagSerial>, argc, argv);
}
