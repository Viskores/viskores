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
#include <vtkm/testing/TestingAlgorithms.h>

#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

int UnitTestTBBAlgorithms(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(
    RunAlgorithmsTests<vtkm::cont::DeviceAdapterTagTBB>, argc, argv);
}
