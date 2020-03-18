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
#include <vtkm/random/Philox.h>

void TestPhiloxRNG2x32x7()
{
  vtkm::random::PhiloxFunctor2x32x7 f2x32x7;
  using counters_type = typename vtkm::random::PhiloxFunctor2x32x7::counters_type;

  // test cases from original Random123 implementation
  VTKM_TEST_ASSERT(f2x32x7({}, {}) == counters_type{ 0x257a3673, 0xcd26be2a });
  VTKM_TEST_ASSERT(f2x32x7({ 0xffffffff, 0xffffffff }, { 0xffffffff }) ==
                   counters_type{ 0xab302c4d, 0x3dc9d239 });
  VTKM_TEST_ASSERT(f2x32x7({ 0x243f6a88, 0x85a308d3 }, { 0x13198a2e }) ==
                   counters_type{ 0xbedbbe6b, 0xe4c770b3 });
}

void TestPhiloxRNG2x32x10()
{
  vtkm::random::PhiloxFunctor2x32x10 f2x32x10;
  using counters_type = typename vtkm::random::PhiloxFunctor2x32x10::counters_type;

  // test cases from original Random123 implementation
  VTKM_TEST_ASSERT(f2x32x10({}, {}) == counters_type{ 0xff1dae59, 0x6cd10df2 });
  VTKM_TEST_ASSERT(f2x32x10({ 0xffffffff, 0xffffffff }, { 0xffffffff }) ==
                   counters_type{ 0x2c3f628b, 0xab4fd7ad });
  VTKM_TEST_ASSERT(f2x32x10({ 0x243f6a88, 0x85a308d3 }, { 0x13198a2e }) ==
                   counters_type{ 0xdd7ce038, 0xf62a4c12 });
}

void TestPhiloxRNG()
{
  TestPhiloxRNG2x32x7();
  TestPhiloxRNG2x32x10();
}

int UnitTestPhiloxRNG(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPhiloxRNG, argc, argv);
}
