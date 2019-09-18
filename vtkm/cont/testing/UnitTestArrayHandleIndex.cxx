//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleIndex.h>

#include <vtkm/cont/testing/Testing.h>

namespace UnitTestArrayHandleIndexNamespace
{

const vtkm::Id ARRAY_SIZE = 10;

void TestArrayHandleIndex()
{
  vtkm::cont::ArrayHandleIndex array(ARRAY_SIZE);
  VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Bad size.");

  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    VTKM_TEST_ASSERT(array.GetPortalConstControl().Get(index) == index,
                     "Index array has unexpected value.");
  }
}

} // namespace UnitTestArrayHandleIndexNamespace

int UnitTestArrayHandleIndex(int argc, char* argv[])
{
  using namespace UnitTestArrayHandleIndexNamespace;
  return vtkm::cont::testing::Testing::Run(TestArrayHandleIndex, argc, argv);
}
