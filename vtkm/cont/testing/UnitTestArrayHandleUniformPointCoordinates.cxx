//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

using Vector3 = vtkm::Vec3f;

const vtkm::Id3 DIMENSIONS(16, 18, 5);
const vtkm::Id NUM_POINTS = 1440;

const Vector3 ORIGIN(-20, 5, -10);
const Vector3 SPACING(10, 1, 0.1f);

void TestArrayHandleUniformPointCoordinates()
{
  std::cout << "Creating ArrayHandleUniformPointCoordinates" << std::endl;

  vtkm::cont::ArrayHandleUniformPointCoordinates arrayHandle(DIMENSIONS, ORIGIN, SPACING);
  VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == NUM_POINTS,
                   "Array computed wrong number of points.");

  std::cout << "Getting array portal." << std::endl;
  vtkm::internal::ArrayPortalUniformPointCoordinates portal = arrayHandle.GetPortalConstControl();
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == NUM_POINTS, "Portal has wrong number of points.");
  VTKM_TEST_ASSERT(portal.GetRange3() == DIMENSIONS, "Portal range is wrong.");

  std::cout << "Checking computed values of portal." << std::endl;
  Vector3 expectedValue;
  vtkm::Id flatIndex = 0;
  vtkm::Id3 blockIndex;
  expectedValue[2] = ORIGIN[2];
  for (blockIndex[2] = 0; blockIndex[2] < DIMENSIONS[2]; blockIndex[2]++)
  {
    expectedValue[1] = ORIGIN[1];
    for (blockIndex[1] = 0; blockIndex[1] < DIMENSIONS[1]; blockIndex[1]++)
    {
      expectedValue[0] = ORIGIN[0];
      for (blockIndex[0] = 0; blockIndex[0] < DIMENSIONS[0]; blockIndex[0]++)
      {
        VTKM_TEST_ASSERT(test_equal(expectedValue, portal.Get(flatIndex)),
                         "Got wrong value for flat index.");

        VTKM_TEST_ASSERT(test_equal(expectedValue, portal.Get(blockIndex)),
                         "Got wrong value for block index.");

        flatIndex++;
        expectedValue[0] += SPACING[0];
      }
      expectedValue[1] += SPACING[1];
    }
    expectedValue[2] += SPACING[2];
  }
}

} // anonymous namespace

int UnitTestArrayHandleUniformPointCoordinates(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleUniformPointCoordinates, argc, argv);
}
