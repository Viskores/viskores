//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleRandomUniformReal.h>
#include <vtkm/cont/testing/Testing.h>

void TestArrayHandleUniformReal()
{
  auto array = vtkm::cont::ArrayHandleRandomUniformReal(10);
  for (vtkm::Id i = 0; i < array.GetNumberOfValues(); ++i)
  {
    auto value = array.ReadPortal().Get(i);
    VTKM_TEST_ASSERT(0.0 <= value && value < 1.0);
  }
}

int UnitTestArrayHandleRandomUniformReal(int argc, char* argv[])
{

  return vtkm::cont::testing::Testing::Run(TestArrayHandleUniformReal, argc, argv);
}
