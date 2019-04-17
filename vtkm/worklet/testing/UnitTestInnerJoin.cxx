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
#include <vtkm/worklet/connectivities/InnerJoin.h>


class TestInnerJoin
{
public:
  template <typename T, typename Storage>
  bool TestArrayHandle(const vtkm::cont::ArrayHandle<T, Storage>& ah,
                       const T* expected,
                       vtkm::Id size) const
  {
    if (size != ah.GetNumberOfValues())
    {
      return false;
    }

    for (vtkm::Id i = 0; i < size; ++i)
    {
      if (ah.GetPortalConstControl().Get(i) != expected[i])
      {
        return false;
      }
    }

    return true;
  }

  void TestTwoArrays() const
  {
    using Algorithm = vtkm::cont::Algorithm;

    std::vector<vtkm::Id> A = { 8, 3, 6, 8, 9, 5, 12, 10, 14 };
    std::vector<vtkm::Id> B = { 7, 11, 9, 8, 5, 1, 0, 5 };

    vtkm::cont::ArrayHandle<vtkm::Id> A_arr = vtkm::cont::make_ArrayHandle(A);
    vtkm::cont::ArrayHandle<vtkm::Id> B_arr = vtkm::cont::make_ArrayHandle(B);
    vtkm::cont::ArrayHandle<vtkm::Id> idxA;
    vtkm::cont::ArrayHandle<vtkm::Id> idxB;

    Algorithm::Copy(vtkm::cont::ArrayHandleIndex(A_arr.GetNumberOfValues()), idxA);
    Algorithm::Copy(vtkm::cont::ArrayHandleIndex(B_arr.GetNumberOfValues()), idxB);

    vtkm::cont::ArrayHandle<vtkm::Id> joinedIndex;
    vtkm::cont::ArrayHandle<vtkm::Id> outA;
    vtkm::cont::ArrayHandle<vtkm::Id> outB;

    vtkm::worklet::connectivity::InnerJoin().Run(A_arr, idxA, B_arr, idxB, joinedIndex, outA, outB);

    vtkm::Id expectedIndex[] = { 5, 5, 8, 8, 9 };
    VTKM_TEST_ASSERT(TestArrayHandle(joinedIndex, expectedIndex, 5), "Wrong joined keys");

    vtkm::Id expectedOutA[] = { 5, 5, 0, 3, 4 };
    VTKM_TEST_ASSERT(TestArrayHandle(outA, expectedOutA, 5), "Wrong joined values");

    vtkm::Id expectedOutB[] = { 4, 7, 3, 3, 2 };
    VTKM_TEST_ASSERT(TestArrayHandle(outB, expectedOutB, 5), "Wrong joined values");
  }

  void operator()() const { this->TestTwoArrays(); }
};

int UnitTestInnerJoin(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestInnerJoin(), argc, argv);
}
