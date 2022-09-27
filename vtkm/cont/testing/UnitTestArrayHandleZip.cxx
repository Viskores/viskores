//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleZip.h>

#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

struct PassThrough : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename InValue, typename OutValue>
  VTKM_EXEC void operator()(const InValue& inValue, OutValue& outValue) const
  {
    outValue = inValue;
  }
};

struct InplaceFunctorPair : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1);

  template <typename T>
  VTKM_EXEC void operator()(vtkm::Pair<T, T>& value) const
  {
    value.second = value.first;
  }
};

struct TestZipAsInput
{
  vtkm::cont::Invoker Invoke;

  template <typename KeyType, typename ValueType>
  VTKM_CONT void operator()(vtkm::Pair<KeyType, ValueType> vtkmNotUsed(pair)) const
  {
    using PairType = vtkm::Pair<KeyType, ValueType>;
    using KeyComponentType = typename vtkm::VecTraits<KeyType>::ComponentType;
    using ValueComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;

    KeyType testKeys[ARRAY_SIZE];
    ValueType testValues[ARRAY_SIZE];

    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      testKeys[i] = KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i));
      testValues[i] = ValueType(static_cast<ValueComponentType>(i));
    }
    vtkm::cont::ArrayHandle<KeyType> keys =
      vtkm::cont::make_ArrayHandle(testKeys, ARRAY_SIZE, vtkm::CopyFlag::Off);
    vtkm::cont::ArrayHandle<ValueType> values =
      vtkm::cont::make_ArrayHandle(testValues, ARRAY_SIZE, vtkm::CopyFlag::Off);

    vtkm::cont::ArrayHandleZip<vtkm::cont::ArrayHandle<KeyType>, vtkm::cont::ArrayHandle<ValueType>>
      zip = vtkm::cont::make_ArrayHandleZip(keys, values);

    vtkm::cont::ArrayHandle<PairType> result;

    this->Invoke(PassThrough{}, zip, result);

    //verify that the control portal works
    auto resultPortal = result.ReadPortal();
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const PairType result_v = resultPortal.Get(i);
      const PairType correct_value(KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i)),
                                   ValueType(static_cast<ValueComponentType>(i)));
      VTKM_TEST_ASSERT(test_equal(result_v, correct_value), "ArrayHandleZip Failed as input");
    }

    zip.ReleaseResources();
  }
};

struct TestZipAsOutput
{
  vtkm::cont::Invoker Invoke;

  template <typename KeyType, typename ValueType>
  VTKM_CONT void operator()(vtkm::Pair<KeyType, ValueType> vtkmNotUsed(pair)) const
  {
    using PairType = vtkm::Pair<KeyType, ValueType>;
    using KeyComponentType = typename vtkm::VecTraits<KeyType>::ComponentType;
    using ValueComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;

    PairType testKeysAndValues[ARRAY_SIZE];
    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      testKeysAndValues[i] = PairType(KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i)),
                                      ValueType(static_cast<ValueComponentType>(i)));
    }
    vtkm::cont::ArrayHandle<PairType> input =
      vtkm::cont::make_ArrayHandle(testKeysAndValues, ARRAY_SIZE, vtkm::CopyFlag::Off);

    vtkm::cont::ArrayHandle<KeyType> result_keys;
    vtkm::cont::ArrayHandle<ValueType> result_values;
    vtkm::cont::ArrayHandleZip<vtkm::cont::ArrayHandle<KeyType>, vtkm::cont::ArrayHandle<ValueType>>
      result_zip = vtkm::cont::make_ArrayHandleZip(result_keys, result_values);

    this->Invoke(PassThrough{}, input, result_zip);

    //now the two arrays we have zipped should have data inside them
    auto keysPortal = result_keys.ReadPortal();
    auto valsPortal = result_values.ReadPortal();
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const KeyType result_key = keysPortal.Get(i);
      const ValueType result_value = valsPortal.Get(i);

      VTKM_TEST_ASSERT(
        test_equal(result_key, KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i))),
        "ArrayHandleZip Failed as input for key");
      VTKM_TEST_ASSERT(test_equal(result_value, ValueType(static_cast<ValueComponentType>(i))),
                       "ArrayHandleZip Failed as input for value");
    }

    // Test filling the zipped array.
    vtkm::cont::printSummary_ArrayHandle(result_zip, std::cout, true);
    PairType fillValue{ TestValue(1, KeyType{}), TestValue(2, ValueType{}) };
    result_zip.Fill(fillValue, 1);
    vtkm::cont::printSummary_ArrayHandle(result_zip, std::cout, true);
    keysPortal = result_keys.ReadPortal();
    valsPortal = result_values.ReadPortal();
    // First entry should be the same.
    VTKM_TEST_ASSERT(
      test_equal(keysPortal.Get(0), KeyType(static_cast<KeyComponentType>(ARRAY_SIZE))));
    VTKM_TEST_ASSERT(test_equal(valsPortal.Get(0), ValueType(static_cast<ValueComponentType>(0))));
    // The rest should be fillValue
    for (vtkm::Id index = 1; index < ARRAY_SIZE; ++index)
    {
      const KeyType result_key = keysPortal.Get(index);
      const ValueType result_value = valsPortal.Get(index);

      VTKM_TEST_ASSERT(test_equal(result_key, fillValue.first));
      VTKM_TEST_ASSERT(test_equal(result_value, fillValue.second));
    }
  }
};

struct TestZipAsInPlace
{
  vtkm::cont::Invoker Invoke;

  template <typename ValueType>
  VTKM_CONT void operator()(ValueType) const
  {
    vtkm::cont::ArrayHandle<ValueType> inputValues;
    inputValues.Allocate(ARRAY_SIZE);
    SetPortal(inputValues.WritePortal());

    vtkm::cont::ArrayHandle<ValueType> outputValues;
    outputValues.Allocate(ARRAY_SIZE);

    this->Invoke(InplaceFunctorPair{}, vtkm::cont::make_ArrayHandleZip(inputValues, outputValues));

    CheckPortal(outputValues.ReadPortal());
  }
};

void Run()
{
  using ZipTypesToTest = vtkm::List<vtkm::Pair<vtkm::UInt8, vtkm::Id>,
                                    vtkm::Pair<vtkm::Float64, vtkm::Vec4ui_8>,
                                    vtkm::Pair<vtkm::Vec3f_32, vtkm::Vec4i_8>>;
  using HandleTypesToTest =
    vtkm::List<vtkm::Id, vtkm::Vec2i_32, vtkm::FloatDefault, vtkm::Vec3f_64>;

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleZip as Input" << std::endl;
  vtkm::testing::Testing::TryTypes(TestZipAsInput(), ZipTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleZip as Output" << std::endl;
  vtkm::testing::Testing::TryTypes(TestZipAsOutput(), ZipTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleZip as In Place" << std::endl;
  vtkm::testing::Testing::TryTypes(TestZipAsInPlace(), HandleTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleZip(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
