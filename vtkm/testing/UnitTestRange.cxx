//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Range.h>

#include <vtkm/testing/Testing.h>

namespace
{

void TestRange()
{
  std::cout << "Empty range." << std::endl;
  vtkm::Range emptyRange;
  VTKM_TEST_ASSERT(!emptyRange.IsNonEmpty(), "Non empty range not empty.");
  VTKM_TEST_ASSERT(test_equal(emptyRange.Length(), 0.0), "Bad length.");

  vtkm::Range emptyRange2;
  VTKM_TEST_ASSERT(!emptyRange2.IsNonEmpty(), "2nd empty range not empty.");
  VTKM_TEST_ASSERT(!emptyRange.Union(emptyRange2).IsNonEmpty(), "Union of empty ranges not empty.");
  emptyRange2.Include(emptyRange);
  VTKM_TEST_ASSERT(!emptyRange2.IsNonEmpty(), "Include empty in empty is not empty.");

  std::cout << "Single value range." << std::endl;
  vtkm::Range singleValueRange(5.0, 5.0);
  VTKM_TEST_ASSERT(singleValueRange.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(test_equal(singleValueRange.Length(), 0.0), "Bad length.");
  VTKM_TEST_ASSERT(test_equal(singleValueRange.Center(), 5), "Bad center.");
  VTKM_TEST_ASSERT(singleValueRange.Contains(5.0), "Does not contain value");
  VTKM_TEST_ASSERT(!singleValueRange.Contains(0.0), "Contains outside");
  VTKM_TEST_ASSERT(!singleValueRange.Contains(10), "Contains outside");

  vtkm::Range unionRange = emptyRange + singleValueRange;
  VTKM_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(test_equal(unionRange.Length(), 0.0), "Bad length.");
  VTKM_TEST_ASSERT(test_equal(unionRange.Center(), 5), "Bad center.");
  VTKM_TEST_ASSERT(unionRange.Contains(5.0), "Does not contain value");
  VTKM_TEST_ASSERT(!unionRange.Contains(0.0), "Contains outside");
  VTKM_TEST_ASSERT(!unionRange.Contains(10), "Contains outside");
  VTKM_TEST_ASSERT(singleValueRange == unionRange, "Union not equal");
  VTKM_TEST_ASSERT(!(singleValueRange != unionRange), "Union not equal");

  std::cout << "Low range." << std::endl;
  vtkm::Range lowRange(-10.0, -5.0);
  VTKM_TEST_ASSERT(lowRange.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(test_equal(lowRange.Length(), 5.0), "Bad length.");
  VTKM_TEST_ASSERT(test_equal(lowRange.Center(), -7.5), "Bad center.");
  VTKM_TEST_ASSERT(!lowRange.Contains(-20), "Contains fail");
  VTKM_TEST_ASSERT(lowRange.Contains(-7), "Contains fail");
  VTKM_TEST_ASSERT(!lowRange.Contains(0), "Contains fail");
  VTKM_TEST_ASSERT(!lowRange.Contains(10), "Contains fail");

  unionRange = singleValueRange + lowRange;
  VTKM_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(test_equal(unionRange.Length(), 15.0), "Bad length.");
  VTKM_TEST_ASSERT(test_equal(unionRange.Center(), -2.5), "Bad center.");
  VTKM_TEST_ASSERT(!unionRange.Contains(-20), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(-7), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(0), "Contains fail");
  VTKM_TEST_ASSERT(!unionRange.Contains(10), "Contains fail");

  std::cout << "High range." << std::endl;
  vtkm::Range highRange(15.0, 20.0);
  VTKM_TEST_ASSERT(highRange.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(test_equal(highRange.Length(), 5.0), "Bad length.");
  VTKM_TEST_ASSERT(test_equal(highRange.Center(), 17.5), "Bad center.");
  VTKM_TEST_ASSERT(!highRange.Contains(-20), "Contains fail");
  VTKM_TEST_ASSERT(!highRange.Contains(-7), "Contains fail");
  VTKM_TEST_ASSERT(!highRange.Contains(0), "Contains fail");
  VTKM_TEST_ASSERT(!highRange.Contains(10), "Contains fail");
  VTKM_TEST_ASSERT(highRange.Contains(17), "Contains fail");
  VTKM_TEST_ASSERT(!highRange.Contains(25), "Contains fail");

  unionRange = highRange.Union(singleValueRange);
  VTKM_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(test_equal(unionRange.Length(), 15.0), "Bad length.");
  VTKM_TEST_ASSERT(test_equal(unionRange.Center(), 12.5), "Bad center.");
  VTKM_TEST_ASSERT(!unionRange.Contains(-20), "Contains fail");
  VTKM_TEST_ASSERT(!unionRange.Contains(-7), "Contains fail");
  VTKM_TEST_ASSERT(!unionRange.Contains(0), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(10), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(17), "Contains fail");
  VTKM_TEST_ASSERT(!unionRange.Contains(25), "Contains fail");

  unionRange.Include(-1);
  VTKM_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(test_equal(unionRange.Length(), 21.0), "Bad length.");
  VTKM_TEST_ASSERT(test_equal(unionRange.Center(), 9.5), "Bad center.");
  VTKM_TEST_ASSERT(!unionRange.Contains(-20), "Contains fail");
  VTKM_TEST_ASSERT(!unionRange.Contains(-7), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(0), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(10), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(17), "Contains fail");
  VTKM_TEST_ASSERT(!unionRange.Contains(25), "Contains fail");

  unionRange.Include(lowRange);
  VTKM_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(test_equal(unionRange.Length(), 30.0), "Bad length.");
  VTKM_TEST_ASSERT(test_equal(unionRange.Center(), 5), "Bad center.");
  VTKM_TEST_ASSERT(!unionRange.Contains(-20), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(-7), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(0), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(10), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(17), "Contains fail");
  VTKM_TEST_ASSERT(!unionRange.Contains(25), "Contains fail");

  std::cout << "Try adding infinity." << std::endl;
  unionRange.Include(vtkm::Infinity64());
  VTKM_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(!unionRange.Contains(-20), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(-7), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(0), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(10), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(17), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(25), "Contains fail");

  std::cout << "Try adding NaN." << std::endl;
  unionRange.Include(vtkm::Nan64());
  VTKM_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VTKM_TEST_ASSERT(!unionRange.Contains(-20), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(-7), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(0), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(10), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(17), "Contains fail");
  VTKM_TEST_ASSERT(unionRange.Contains(25), "Contains fail");
}

} // anonymous namespace

int UnitTestRange(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestRange, argc, argv);
}
