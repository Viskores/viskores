//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Bounds.h>

#include <vtkm/testing/Testing.h>

namespace
{

void TestBounds()
{
  using Vec3 = vtkm::Vec3f_64;

  std::cout << "Empty bounds." << std::endl;
  vtkm::Bounds emptyBounds;
  VTKM_TEST_ASSERT(!emptyBounds.IsNonEmpty(), "Non empty bounds not empty.");

  vtkm::Bounds emptyBounds2;
  VTKM_TEST_ASSERT(!emptyBounds2.IsNonEmpty(), "2nd empty bounds not empty.");
  VTKM_TEST_ASSERT(!emptyBounds.Union(emptyBounds2).IsNonEmpty(),
                   "Union of empty bounds not empty.");
  emptyBounds2.Include(emptyBounds);
  VTKM_TEST_ASSERT(!emptyBounds2.IsNonEmpty(), "Include empty in empty is not empty.");

  std::cout << "Single value bounds." << std::endl;
  vtkm::Bounds singleValueBounds(1.0, 1.0, 2.0, 2.0, 3.0, 3.0);
  VTKM_TEST_ASSERT(singleValueBounds.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(test_equal(singleValueBounds.Center(), Vec3(1, 2, 3)), "Bad center");
  VTKM_TEST_ASSERT(singleValueBounds.Contains(Vec3(1, 2, 3)), "Contains fail");
  VTKM_TEST_ASSERT(!singleValueBounds.Contains(Vec3(0, 0, 0)), "Contains fail");
  VTKM_TEST_ASSERT(!singleValueBounds.Contains(Vec3(2, 2, 2)), "contains fail");
  VTKM_TEST_ASSERT(!singleValueBounds.Contains(Vec3(5, 5, 5)), "contains fail");

  vtkm::Bounds unionBounds = emptyBounds + singleValueBounds;
  VTKM_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(test_equal(unionBounds.Center(), Vec3(1, 2, 3)), "Bad center");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(1, 2, 3)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(0, 0, 0)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(2, 2, 2)), "contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(5, 5, 5)), "contains fail");
  VTKM_TEST_ASSERT(singleValueBounds == unionBounds, "Union not equal");

  std::cout << "Low bounds." << std::endl;
  vtkm::Bounds lowBounds(Vec3(-10, -5, -1), Vec3(-5, -2, 0));
  VTKM_TEST_ASSERT(lowBounds.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(test_equal(lowBounds.Center(), Vec3(-7.5, -3.5, -0.5)), "Bad center");
  VTKM_TEST_ASSERT(!lowBounds.Contains(Vec3(-20)), "Contains fail");
  VTKM_TEST_ASSERT(!lowBounds.Contains(Vec3(-2)), "Contains fail");
  VTKM_TEST_ASSERT(lowBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VTKM_TEST_ASSERT(!lowBounds.Contains(Vec3(0)), "Contains fail");
  VTKM_TEST_ASSERT(!lowBounds.Contains(Vec3(10)), "Contains fail");

  unionBounds = singleValueBounds + lowBounds;
  VTKM_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(10)), "Contains fail");

  std::cout << "High bounds." << std::endl;
  vtkm::Float64 highBoundsArray[6] = { 15.0, 20.0, 2.0, 5.0, 5.0, 10.0 };
  vtkm::Bounds highBounds(highBoundsArray);
  VTKM_TEST_ASSERT(highBounds.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(!highBounds.Contains(Vec3(-20)), "Contains fail");
  VTKM_TEST_ASSERT(!highBounds.Contains(Vec3(-2)), "Contains fail");
  VTKM_TEST_ASSERT(!highBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VTKM_TEST_ASSERT(!highBounds.Contains(Vec3(0)), "Contains fail");
  VTKM_TEST_ASSERT(!highBounds.Contains(Vec3(4)), "Contains fail");
  VTKM_TEST_ASSERT(highBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  VTKM_TEST_ASSERT(!highBounds.Contains(Vec3(25)), "Contains fail");

  unionBounds = highBounds.Union(singleValueBounds);
  VTKM_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(0)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(25)), "Contains fail");

  unionBounds.Include(Vec3(-1));
  VTKM_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(25)), "Contains fail");

  unionBounds.Include(lowBounds);
  VTKM_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(25)), "Contains fail");

  std::cout << "Try adding infinity." << std::endl;
  unionBounds.Include(Vec3(vtkm::Infinity64()));
  VTKM_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(25)), "Contains fail");

  std::cout << "Try adding NaN." << std::endl;
  unionBounds.Include(Vec3(vtkm::Nan64()));
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  VTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  VTKM_TEST_ASSERT(unionBounds.Contains(Vec3(25)), "Contains fail");
}

} // anonymous namespace

int UnitTestBounds(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestBounds, argc, argv);
}
