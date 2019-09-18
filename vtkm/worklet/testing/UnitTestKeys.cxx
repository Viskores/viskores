//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/Keys.h>

#include <vtkm/cont/ArrayCopy.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 1033;
static constexpr vtkm::Id NUM_UNIQUE = ARRAY_SIZE / 10;

template <typename KeyPortal, typename IdPortal, typename IdComponentPortal>
void CheckKeyReduce(const KeyPortal& originalKeys,
                    const KeyPortal& uniqueKeys,
                    const IdPortal& sortedValuesMap,
                    const IdPortal& offsets,
                    const IdComponentPortal& counts)
{
  using KeyType = typename KeyPortal::ValueType;
  vtkm::Id originalSize = originalKeys.GetNumberOfValues();
  vtkm::Id uniqueSize = uniqueKeys.GetNumberOfValues();
  VTKM_TEST_ASSERT(originalSize == sortedValuesMap.GetNumberOfValues(), "Inconsistent array size.");
  VTKM_TEST_ASSERT(uniqueSize == offsets.GetNumberOfValues(), "Inconsistent array size.");
  VTKM_TEST_ASSERT(uniqueSize == counts.GetNumberOfValues(), "Inconsistent array size.");

  for (vtkm::Id uniqueIndex = 0; uniqueIndex < uniqueSize; uniqueIndex++)
  {
    KeyType key = uniqueKeys.Get(uniqueIndex);
    vtkm::Id offset = offsets.Get(uniqueIndex);
    vtkm::IdComponent groupCount = counts.Get(uniqueIndex);
    for (vtkm::IdComponent groupIndex = 0; groupIndex < groupCount; groupIndex++)
    {
      vtkm::Id originalIndex = sortedValuesMap.Get(offset + groupIndex);
      KeyType originalKey = originalKeys.Get(originalIndex);
      VTKM_TEST_ASSERT(key == originalKey, "Bad key lookup.");
    }
  }
}

template <typename KeyType>
void TryKeyType(KeyType)
{
  KeyType keyBuffer[ARRAY_SIZE];
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    keyBuffer[index] = TestValue(index % NUM_UNIQUE, KeyType());
  }

  vtkm::cont::ArrayHandle<KeyType> keyArray = vtkm::cont::make_ArrayHandle(keyBuffer, ARRAY_SIZE);

  vtkm::cont::ArrayHandle<KeyType> sortedKeys;
  vtkm::cont::ArrayCopy(keyArray, sortedKeys);

  vtkm::worklet::Keys<KeyType> keys(sortedKeys);
  VTKM_TEST_ASSERT(keys.GetInputRange() == NUM_UNIQUE, "Keys has bad input range.");

  CheckKeyReduce(keyArray.GetPortalConstControl(),
                 keys.GetUniqueKeys().GetPortalConstControl(),
                 keys.GetSortedValuesMap().GetPortalConstControl(),
                 keys.GetOffsets().GetPortalConstControl(),
                 keys.GetCounts().GetPortalConstControl());
}

void TestKeys()
{
  std::cout << "Testing vtkm::Id keys." << std::endl;
  TryKeyType(vtkm::Id());

  std::cout << "Testing vtkm::IdComponent keys." << std::endl;
  TryKeyType(vtkm::IdComponent());

  std::cout << "Testing vtkm::UInt8 keys." << std::endl;
  TryKeyType(vtkm::UInt8());

  std::cout << "Testing vtkm::Id3 keys." << std::endl;
  TryKeyType(vtkm::Id3());
}

} // anonymous namespace

int UnitTestKeys(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestKeys, argc, argv);
}
