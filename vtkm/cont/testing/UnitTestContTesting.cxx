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

#include <vtkm/Assert.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestFail()
{
  VTKM_TEST_FAIL("I expect this error.");
}

void BadTestAssert()
{
  VTKM_TEST_ASSERT(0 == 1, "I expect this error.");
}

void GoodAssert()
{
  VTKM_TEST_ASSERT(1 == 1, "Always true.");
  VTKM_ASSERT(1 == 1);
}

} // anonymous namespace

int UnitTestContTesting(int, char* [])
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

  std::cout << "-------\nThis call should pass." << std::endl;
  // This is what your main function typically looks like.
  return vtkm::cont::testing::Testing::Run(GoodAssert);
}
