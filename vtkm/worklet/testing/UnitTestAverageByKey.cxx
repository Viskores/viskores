//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/AverageByKey.h>

#include <vtkm/Hash.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleCounting.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static constexpr vtkm::Id NUM_UNIQUE = 100;
static constexpr vtkm::Id NUM_PER_GROUP = 10;
static constexpr vtkm::Id ARRAY_SIZE = NUM_UNIQUE * NUM_PER_GROUP;

template <typename KeyArray, typename ValueArray>
void CheckAverageByKey(const KeyArray& uniqueKeys, const ValueArray& averagedValues)
{
  VTKM_IS_ARRAY_HANDLE(KeyArray);
  VTKM_IS_ARRAY_HANDLE(ValueArray);

  using KeyType = typename KeyArray::ValueType;

  VTKM_TEST_ASSERT(uniqueKeys.GetNumberOfValues() == NUM_UNIQUE, "Bad number of keys.");
  VTKM_TEST_ASSERT(averagedValues.GetNumberOfValues() == NUM_UNIQUE, "Bad number of values.");

  // We expect the unique keys to be sorted, and for the test values to be in order.
  auto keyPortal = uniqueKeys.GetPortalConstControl();
  auto valuePortal = averagedValues.GetPortalConstControl();
  for (vtkm::Id index = 0; index < NUM_UNIQUE; ++index)
  {
    VTKM_TEST_ASSERT(keyPortal.Get(index) == TestValue(index % NUM_UNIQUE, KeyType()),
                     "Unexpected key.");

    vtkm::FloatDefault expectedAverage = static_cast<vtkm::FloatDefault>(
      NUM_PER_GROUP * ((NUM_PER_GROUP - 1) * NUM_PER_GROUP) / 2 + index);
    VTKM_TEST_ASSERT(test_equal(expectedAverage, valuePortal.Get(index)), "Bad average.");
  }
}

template <typename KeyType>
void TryKeyType(KeyType)
{
  std::cout << "Testing with " << vtkm::testing::TypeName<KeyType>::Name() << " keys." << std::endl;

  // Create key array
  KeyType keyBuffer[ARRAY_SIZE];
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    keyBuffer[index] = TestValue(index % NUM_UNIQUE, KeyType());
  }
  vtkm::cont::ArrayHandle<KeyType> keysArray = vtkm::cont::make_ArrayHandle(keyBuffer, ARRAY_SIZE);

  // Create Keys object
  vtkm::cont::ArrayHandle<KeyType> sortedKeys;
  vtkm::cont::ArrayCopy(keysArray, sortedKeys);
  vtkm::worklet::Keys<KeyType> keys(sortedKeys);
  VTKM_TEST_ASSERT(keys.GetInputRange() == NUM_UNIQUE, "Keys has bad input range.");

  // Create values array
  vtkm::cont::ArrayHandleCounting<vtkm::FloatDefault> valuesArray(0.0f, 1.0f, ARRAY_SIZE);

  std::cout << "  Try average with Keys object" << std::endl;
  CheckAverageByKey(keys.GetUniqueKeys(), vtkm::worklet::AverageByKey::Run(keys, valuesArray));

  std::cout << "  Try average with device adapter's reduce by keys" << std::endl;
  vtkm::cont::ArrayHandle<KeyType> outputKeys;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outputValues;
  vtkm::worklet::AverageByKey::Run(keysArray, valuesArray, outputKeys, outputValues);
  CheckAverageByKey(outputKeys, outputValues);
}

void DoTest()
{
  TryKeyType(vtkm::Id());
  TryKeyType(vtkm::IdComponent());
  TryKeyType(vtkm::UInt8());
  TryKeyType(vtkm::HashType());
  TryKeyType(vtkm::Id3());
}

} // anonymous namespace

int UnitTestAverageByKey(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
