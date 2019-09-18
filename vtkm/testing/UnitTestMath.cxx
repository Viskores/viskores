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
#include <vtkm/testing/TestingMath.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>

int UnitTestMath(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(
    UnitTestMathNamespace::RunMathTests<vtkm::cont::DeviceAdapterTagSerial>, argc, argv);
}
