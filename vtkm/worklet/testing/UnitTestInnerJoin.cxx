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
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/connectivities/InnerJoin.h>


class TestInnerJoin
{
public:
  static bool TestJoinedValues(const vtkm::cont::ArrayHandle<vtkm::Id>& computedValuesArray,
                               const vtkm::cont::ArrayHandle<vtkm::Id>& expectedValuesArray,
                               const vtkm::cont::ArrayHandle<vtkm::Id>& originalKeysArray)
  {
    auto computedValues = computedValuesArray.ReadPortal();
    auto expectedValues = expectedValuesArray.ReadPortal();
    auto originalKeys = originalKeysArray.ReadPortal();
    if (computedValues.GetNumberOfValues() != expectedValues.GetNumberOfValues())
    {
      return false;
    }

    for (vtkm::Id valueIndex = 0; valueIndex < computedValues.GetNumberOfValues(); ++valueIndex)
    {
      vtkm::Id computed = computedValues.Get(valueIndex);
      vtkm::Id expected = expectedValues.Get(valueIndex);

      // The join algorithm uses some key/value sorts that are unstable. Thus, for keys
      // that are repeated in the original input, the computed and expected values may be
      // swapped in the results associated with those keys. To test correctly, the values
      // we computed for are actually indices into the original keys array. Thus, if both
      // computed and expected are different indices that point to the same original key,
      // then the algorithm is still correct.
      vtkm::Id computedKey = originalKeys.Get(computed);
      vtkm::Id expectedKey = originalKeys.Get(expected);
      if (computedKey != expectedKey)
      {
        return false;
      }
    }

    return true;
  }

  void TestTwoArrays() const
  {
    vtkm::cont::ArrayHandle<vtkm::Id> keysAOriginal =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 8, 3, 6, 8, 9, 5, 12, 10, 14 });
    vtkm::cont::ArrayHandle<vtkm::Id> keysBOriginal =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 7, 11, 9, 8, 5, 1, 0, 5 });

    vtkm::cont::ArrayHandle<vtkm::Id> keysA;
    vtkm::cont::ArrayHandle<vtkm::Id> keysB;
    vtkm::cont::ArrayHandle<vtkm::Id> valuesA;
    vtkm::cont::ArrayHandle<vtkm::Id> valuesB;

    vtkm::cont::ArrayCopy(keysAOriginal, keysA);
    vtkm::cont::ArrayCopy(keysBOriginal, keysB);
    vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(keysA.GetNumberOfValues()), valuesA);
    vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(keysB.GetNumberOfValues()), valuesB);

    vtkm::cont::ArrayHandle<vtkm::Id> joinedIndex;
    vtkm::cont::ArrayHandle<vtkm::Id> outA;
    vtkm::cont::ArrayHandle<vtkm::Id> outB;

    vtkm::worklet::connectivity::InnerJoin().Run(
      keysA, valuesA, keysB, valuesB, joinedIndex, outA, outB);

    vtkm::cont::ArrayHandle<vtkm::Id> expectedIndex =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 5, 5, 8, 8, 9 });
    VTKM_TEST_ASSERT(test_equal_portals(joinedIndex.ReadPortal(), expectedIndex.ReadPortal()));

    vtkm::cont::ArrayHandle<vtkm::Id> expectedOutA =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 5, 5, 0, 3, 4 });
    VTKM_TEST_ASSERT(TestJoinedValues(outA, expectedOutA, keysAOriginal));

    vtkm::cont::ArrayHandle<vtkm::Id> expectedOutB =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 4, 7, 3, 3, 2 });
    VTKM_TEST_ASSERT(TestJoinedValues(outB, expectedOutB, keysBOriginal));
  }

  void operator()() const { this->TestTwoArrays(); }
};

int UnitTestInnerJoin(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestInnerJoin(), argc, argv);
}
