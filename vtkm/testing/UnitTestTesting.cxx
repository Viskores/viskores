//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

void Fail2()
{
  vtkm::Id num = 5;
  VTKM_TEST_FAIL("I can provide a number: ", num);
}

void BadAssert()
{
  VTKM_TEST_ASSERT(0 == 1, "I expect this error.");
}

void BadAssert2()
{
  vtkm::Id num1 = 0;
  vtkm::Id num2 = 1;
  VTKM_TEST_ASSERT(num1 == num2, "num 1 is ", num1, "; num 2 is ", num2);
}

void BadAssert3()
{
  VTKM_TEST_ASSERT(0 == 1);
}

void GoodAssert()
{
  VTKM_TEST_ASSERT(1 == 1, "Always true.");
  VTKM_TEST_ASSERT(1 == 1);
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

int UnitTestTesting(int argc, char* argv[])
{
  std::cout << "This call should fail." << std::endl;
  if (vtkm::testing::Testing::Run(Fail, argc, argv) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }
  std::cout << "This call should fail." << std::endl;
  if (vtkm::testing::Testing::Run(Fail2, argc, argv) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }
  std::cout << "This call should fail." << std::endl;
  if (vtkm::testing::Testing::Run(BadAssert, argc, argv) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }
  std::cout << "This call should fail." << std::endl;
  if (vtkm::testing::Testing::Run(BadAssert2, argc, argv) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }
  std::cout << "This call should fail." << std::endl;
  if (vtkm::testing::Testing::Run(BadAssert3, argc, argv) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }

  std::cout << "This call should pass." << std::endl;
  // This is what your main function typically looks like.
  return vtkm::testing::Testing::Run(CleanTests, argc, argv);
}
