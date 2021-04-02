//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/FileUtils.h>

#include <string>

using namespace vtkm::io;

namespace
{
void TestEndsWith()
{
  VTKM_TEST_ASSERT(EndsWith("checking.val", ".val"), "Ending did not match '.val'");
  VTKM_TEST_ASSERT(EndsWith("special_char$&#*", "_char$&#*"), "Ending did not match '_char$&#*'");
  VTKM_TEST_ASSERT(!EndsWith("wrong_ending", "fing"), "Ending did not match 'fing'");
  VTKM_TEST_ASSERT(!EndsWith("too_long", "ending_too_long"),
                   "Ending did not match 'ending_too_long'");
  VTKM_TEST_ASSERT(EndsWith("empty_string", ""), "Ending did not match ''");
}

void TestUtils()
{
  TestEndsWith();
}
}

int UnitTestFileUtils(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestUtils, argc, argv);
}
