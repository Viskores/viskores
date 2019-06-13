//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/DispatcherReduceByKey.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

#include <vtkm/worklet/Keys.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DeviceAdapterTag.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

#define STRINGIFY(x) STRINGIFY_IMPL(x)
#define STRINGIFY_IMPL(x) #x

#define TEST_ASSERT_WORKLET(condition)                                                             \
  do                                                                                               \
  {                                                                                                \
    if (!(condition))                                                                              \
    {                                                                                              \
      this->RaiseError("Test assert failed: " #condition "\n" __FILE__ ":" STRINGIFY(__LINE__));   \
      return;                                                                                      \
    }                                                                                              \
  } while (false)

#define ARRAY_SIZE 1033
#define GROUP_SIZE 10
#define NUM_UNIQUE (vtkm::Id)(ARRAY_SIZE / GROUP_SIZE)

struct CheckKeyValuesWorklet : vtkm::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn keys,
                                ValuesIn keyMirror,
                                ValuesIn indexValues,
                                ValuesInOut valuesToModify,
                                ValuesOut writeKey);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, WorkIndex, ValueCount);
  using InputDomain = _1;

  template <typename T,
            typename KeyMirrorVecType,
            typename IndexValuesVecType,
            typename ValuesToModifyVecType,
            typename WriteKeysVecType>
  VTKM_EXEC void operator()(const T& key,
                            const KeyMirrorVecType& keyMirror,
                            const IndexValuesVecType& valueIndices,
                            ValuesToModifyVecType& valuesToModify,
                            WriteKeysVecType& writeKey,
                            vtkm::Id workIndex,
                            vtkm::IdComponent numValues) const
  {
    // These tests only work if keys are in sorted order, which is how we group
    // them.

    TEST_ASSERT_WORKLET(key == TestValue(workIndex, T()));

    TEST_ASSERT_WORKLET(numValues >= GROUP_SIZE);
    TEST_ASSERT_WORKLET(keyMirror.GetNumberOfComponents() == numValues);
    TEST_ASSERT_WORKLET(valueIndices.GetNumberOfComponents() == numValues);
    TEST_ASSERT_WORKLET(valuesToModify.GetNumberOfComponents() == numValues);
    TEST_ASSERT_WORKLET(writeKey.GetNumberOfComponents() == numValues);

    for (vtkm::IdComponent iComponent = 0; iComponent < numValues; iComponent++)
    {
      TEST_ASSERT_WORKLET(test_equal(keyMirror[iComponent], key));
      TEST_ASSERT_WORKLET(valueIndices[iComponent] % NUM_UNIQUE == workIndex);

      T value = valuesToModify[iComponent];
      valuesToModify[iComponent] = static_cast<T>(key + value);

      writeKey[iComponent] = key;
    }
  }
};

struct CheckReducedValuesWorklet : vtkm::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn,
                                ReducedValuesOut extractKeys,
                                ReducedValuesIn indexReference,
                                ReducedValuesInOut copyKeyPair);
  using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);

  template <typename T>
  VTKM_EXEC void operator()(const T& key,
                            T& reducedValueOut,
                            vtkm::Id indexReference,
                            vtkm::Pair<T, T>& copyKeyPair,
                            vtkm::Id workIndex) const
  {
    // This check only work if keys are in sorted order, which is how we group
    // them.
    TEST_ASSERT_WORKLET(key == TestValue(workIndex, T()));

    reducedValueOut = key;

    TEST_ASSERT_WORKLET(indexReference == workIndex);

    TEST_ASSERT_WORKLET(copyKeyPair.first == key);
    copyKeyPair.second = key;
  }
};

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

  vtkm::cont::ArrayHandle<KeyType> valuesToModify;
  valuesToModify.Allocate(ARRAY_SIZE);
  SetPortal(valuesToModify.GetPortalControl());

  vtkm::cont::ArrayHandle<KeyType> writeKey;

  vtkm::worklet::DispatcherReduceByKey<CheckKeyValuesWorklet> dispatcherCheckKeyValues;
  dispatcherCheckKeyValues.Invoke(
    keys, keyArray, vtkm::cont::ArrayHandleIndex(ARRAY_SIZE), valuesToModify, writeKey);

  VTKM_TEST_ASSERT(valuesToModify.GetNumberOfValues() == ARRAY_SIZE, "Bad array size.");
  VTKM_TEST_ASSERT(writeKey.GetNumberOfValues() == ARRAY_SIZE, "Bad array size.");
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    KeyType key = TestValue(index % NUM_UNIQUE, KeyType());
    KeyType value = TestValue(index, KeyType());

    VTKM_TEST_ASSERT(test_equal(static_cast<KeyType>(key + value),
                                valuesToModify.GetPortalConstControl().Get(index)),
                     "Bad in/out value.");

    VTKM_TEST_ASSERT(test_equal(key, writeKey.GetPortalConstControl().Get(index)),
                     "Bad out value.");
  }

  vtkm::cont::ArrayHandle<KeyType> keyPairIn;
  keyPairIn.Allocate(NUM_UNIQUE);
  SetPortal(keyPairIn.GetPortalControl());

  vtkm::cont::ArrayHandle<KeyType> keyPairOut;
  keyPairOut.Allocate(NUM_UNIQUE);

  vtkm::worklet::DispatcherReduceByKey<CheckReducedValuesWorklet> dispatcherCheckReducedValues;
  dispatcherCheckReducedValues.Invoke(keys,
                                      writeKey,
                                      vtkm::cont::ArrayHandleIndex(NUM_UNIQUE),
                                      vtkm::cont::make_ArrayHandleZip(keyPairIn, keyPairOut));

  VTKM_TEST_ASSERT(writeKey.GetNumberOfValues() == NUM_UNIQUE,
                   "Reduced values output not sized correctly.");
  CheckPortal(writeKey.GetPortalConstControl());

  CheckPortal(keyPairOut.GetPortalConstControl());
}

void TestReduceByKey(vtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Map Field on device adapter: " << id.GetName() << std::endl;

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

int UnitTestWorkletReduceByKey(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::RunOnDevice(TestReduceByKey, argc, argv);
}
