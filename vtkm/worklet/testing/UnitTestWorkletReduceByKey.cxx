//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#include <vtkm/worklet/DispatcherReduceByKey.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

#include <vtkm/worklet/Keys.h>

#include <vtkm/cont/testing/Testing.h>

namespace {

#define STRINGIFY(x) STRINGIFY_IMPL(x)
#define STRINGIFY_IMPL(x) #x

#define IMPL_TEST_ASSERT_WORKLET(condition, file, line) \
  this->RaiseError("Test assert failed: " #condition \
                   "\n" #file ":" #line) \

#define TEST_ASSERT_WORKLET(condition) \
  do { \
    if (!(condition)) \
    { \
      this->RaiseError("Test assert failed: " #condition \
                       "\n" __FILE__ ":" STRINGIFY(__LINE__)); \
    } \
  } while (false)

static const vtkm::Id ARRAY_SIZE = 1033;
static const vtkm::IdComponent GROUP_SIZE = 10;
static const vtkm::Id NUM_UNIQUE = ARRAY_SIZE/GROUP_SIZE;

struct CheckReduceByKeyWorklet : vtkm::worklet::WorkletReduceByKey
{
  typedef void ControlSignature(KeysIn keys,
                                ValuesIn<> keyMirror,
                                ValuesIn<> indexValues,
                                ValuesInOut<> valuesToModify,
                                ValuesOut<> writeKey);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, WorkIndex, ValueCount);
  typedef _1 InputDomain;

  template<typename T,
           typename KeyMirrorVecType,
           typename IndexValuesVecType,
           typename ValuesToModifyVecType,
           typename WriteKeysVecType>
  VTKM_EXEC
  void operator()(const T &key,
                  const KeyMirrorVecType &keyMirror,
                  const IndexValuesVecType &valueIndices,
                  ValuesToModifyVecType &valuesToModify,
                  WriteKeysVecType &writeKey,
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
      TEST_ASSERT_WORKLET(valueIndices[iComponent]%NUM_UNIQUE == workIndex);

      T value = valuesToModify[iComponent];
      valuesToModify[iComponent] = static_cast<T>(key + value);

      writeKey[iComponent] = key;
    }
  }
};

template<typename KeyType>
void TryKeyType(KeyType)
{
  KeyType keyBuffer[ARRAY_SIZE];
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    keyBuffer[index] = TestValue(index%NUM_UNIQUE, KeyType());
  }

  vtkm::cont::ArrayHandle<KeyType> keyArray =
      vtkm::cont::make_ArrayHandle(keyBuffer, ARRAY_SIZE);

  vtkm::worklet::Keys<KeyType> keys(keyArray,
                                    VTKM_DEFAULT_DEVICE_ADAPTER_TAG());

  vtkm::cont::ArrayHandle<KeyType> valuesToModify;
  valuesToModify.Allocate(ARRAY_SIZE);
  SetPortal(valuesToModify.GetPortalControl());

  vtkm::cont::ArrayHandle<KeyType> writeKey;

  vtkm::worklet::DispatcherReduceByKey<CheckReduceByKeyWorklet> dispatcher;
  dispatcher.Invoke(keys,
                    keyArray,
                    vtkm::cont::ArrayHandleIndex(ARRAY_SIZE),
                    valuesToModify,
                    writeKey);

  VTKM_TEST_ASSERT(valuesToModify.GetNumberOfValues() == ARRAY_SIZE,
                   "Bad array size.");
  VTKM_TEST_ASSERT(writeKey.GetNumberOfValues() == ARRAY_SIZE,
                   "Bad array size.");
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    KeyType key = TestValue(index%NUM_UNIQUE, KeyType());
    KeyType value = TestValue(index, KeyType());

    VTKM_TEST_ASSERT(
          test_equal(static_cast<KeyType>(key+value),
                     valuesToModify.GetPortalConstControl().Get(index)),
          "Bad in/out value.");

    VTKM_TEST_ASSERT(
          test_equal(key, writeKey.GetPortalConstControl().Get(index)),
          "Bad out value.");
  }
}

void TestReduceByKey()
{
  typedef vtkm::cont::DeviceAdapterTraits<
                    VTKM_DEFAULT_DEVICE_ADAPTER_TAG> DeviceAdapterTraits;
  std::cout << "Testing Map Field on device adapter: "
            << DeviceAdapterTraits::GetName() << std::endl;

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

int UnitTestWorkletReduceByKey(int, char*[])
{
  return vtkm::cont::testing::Testing::Run(TestReduceByKey);
}
