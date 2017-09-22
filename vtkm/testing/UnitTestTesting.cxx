//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

// This meta-test makes sure that the testing environment is properly reporting
// errors.

#include <vtkm/testing/Testing.h>

namespace
{

void Fail()
{
  VTKM_TEST_FAIL("I expect this error.");
}

void BadAssert()
{
  VTKM_TEST_ASSERT(0 == 1, "I expect this error.");
}

void GoodAssert()
{
  VTKM_TEST_ASSERT(1 == 1, "Always true.");
}

void TestTestEqual()
{
  VTKM_TEST_ASSERT(test_equal(2.0, 1.9999999), "These should be close enough.");
  VTKM_TEST_ASSERT(!test_equal(2.0, 1.999), "These should not be close enough.");
}

// All tests that should not raise a failure.
void CleanTests()
{
  GoodAssert();
  TestTestEqual();
}

} // anonymous namespace

int UnitTestTesting(int, char* [])
{
  std::cout << "This call should fail." << std::endl;
  if (vtkm::testing::Testing::Run(Fail) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }
  std::cout << "This call should fail." << std::endl;
  if (vtkm::testing::Testing::Run(BadAssert) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }

  std::cout << "This call should pass." << std::endl;
  // This is what your main function typically looks like.
  return vtkm::testing::Testing::Run(CleanTests);
}
