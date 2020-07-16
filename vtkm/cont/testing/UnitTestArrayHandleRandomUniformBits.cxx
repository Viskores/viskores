//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleRandomUniformBits.h>
#include <vtkm/cont/testing/Testing.h>

void TestArrayHandleRandomUniformBits()
{
  auto actual0 = vtkm::cont::ArrayHandleRandomUniformBits(10, { 0 });
  // result from Random123 sample implementation of philox2x32x10
  auto expected0 = vtkm::cont::make_ArrayHandle<vtkm::UInt64>({ 0x6cd10df2ff1dae59,
                                                                0x5f3adb6bdcdce855,
                                                                0x3fbb6394049f6998,
                                                                0xbd592d1202a74512,
                                                                0x8a115b62c08084ef,
                                                                0x1411803b3bb7eefa,
                                                                0x7d138a2280027d0e,
                                                                0x318a7703a1da82c5,
                                                                0xdcd79c6998975579,
                                                                0x6cb1a07c91f81109 });


  auto result = vtkm::cont::testing::test_equal_ArrayHandles(actual0, expected0);
  VTKM_TEST_ASSERT(result, result.GetMergedMessage());

  // initialize with seed = 100, could be "iteration number" in actual use case.
  auto actual100 = vtkm::cont::ArrayHandleRandomUniformBits(10, { 100 });
  // result from Random123 sample implementation of philox2x32x10
  auto expected100 = vtkm::cont::make_ArrayHandle<vtkm::UInt64>({ 0xbd35360836122ea3,
                                                                  0xe033b74acce7aa5f,
                                                                  0xc0fbb65cba93ecd7,
                                                                  0xe3fee2812b77e480,
                                                                  0x92e5c7d563767971,
                                                                  0xd99e952fb054fc19,
                                                                  0xb8f2adc12094ad29,
                                                                  0xb7dcb35fea8c27ac,
                                                                  0x9c7b779e88270c45,
                                                                  0x7325b123dc32e01d });
  auto result100 = vtkm::cont::testing::test_equal_ArrayHandles(actual100, expected100);
  VTKM_TEST_ASSERT(result, result.GetMergedMessage());
}

int UnitTestArrayHandleRandomUniformBits(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleRandomUniformBits, argc, argv);
}
