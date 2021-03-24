//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/worklet/connectivities/UnionFind.h>

void TestLinear()
{
  const vtkm::Id N = 100;
  auto counting = vtkm::cont::make_ArrayHandleCounting(-1, 1, N - 1);

  vtkm::cont::ArrayHandle<vtkm::Id> parents;
  vtkm::cont::ArrayCopy(counting, parents);
  parents.WritePortal().Set(0, 0);

  vtkm::cont::Invoker invoker;
  invoker(vtkm::worklet::connectivity::PointerJumping{}, parents);
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, N - 1), parents));
}

void TestPointerJumping()
{
  TestLinear();
}

int UnitTestPointerJumping(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPointerJumping, argc, argv);
}
