//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

// This meta-test makes sure that the testing environment is properly reporting
// errors.

#include <vtkm/cont/Assert.h>

#include <vtkm/cont/testing/Testing.h>

namespace {

void TestFail()
{
  VTKM_TEST_FAIL("I expect this error.");
}

void BadTestAssert()
{
  VTKM_TEST_ASSERT(0 == 1, "I expect this error.");
}

void BadAssert()
{
  VTKM_ASSERT_CONT(0 == 1);
}

void GoodAssert()
{
  VTKM_TEST_ASSERT(1 == 1, "Always true.");
  VTKM_ASSERT_CONT(1 == 1);
}

} // anonymous namespace

int UnitTestContTesting(int, char *[])
{
  std::cout << "-------\nThis call should fail." << std::endl;
  if (vtkm::cont::testing::Testing::Run(TestFail) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }
  std::cout << "-------\nThis call should fail." << std::endl;
  if (vtkm::cont::testing::Testing::Run(BadTestAssert) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }

  //VTKM_ASSERT_CONT only is a valid call when you are building with debug
#ifndef NDEBUG
  int expectedResult=0;
#else
  int expectedResult=1;
#endif

  std::cout << "-------\nThis call should fail on debug builds." << std::endl;
  if (vtkm::cont::testing::Testing::Run(BadAssert) == expectedResult)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }

  std::cout << "-------\nThis call should pass." << std::endl;
  // This is what your main function typically looks like.
  return vtkm::cont::testing::Testing::Run(GoodAssert);
}
