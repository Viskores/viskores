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
#include <vtkm/cont/ArrayCopyDevice.h>
#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/UncertainArrayHandle.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/TypeTraits.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

vtkm::cont::UnknownArrayHandle MakeComparable(const vtkm::cont::UnknownArrayHandle& array,
                                              std::false_type)
{
  return array;
}

template <typename T>
vtkm::cont::UnknownArrayHandle MakeComparable(const vtkm::cont::ArrayHandle<T>& array,
                                              std::true_type)
{
  return array;
}

template <typename ArrayType>
vtkm::cont::UnknownArrayHandle MakeComparable(const ArrayType& array, std::true_type)
{
  vtkm::cont::ArrayHandle<typename ArrayType::ValueType> simpleArray;
  vtkm::cont::ArrayCopyDevice(array, simpleArray);
  return simpleArray;
}

void TestValuesImpl(const vtkm::cont::UnknownArrayHandle& refArray,
                    const vtkm::cont::UnknownArrayHandle& testArray)
{
  auto result = test_equal_ArrayHandles(refArray, testArray);
  VTKM_TEST_ASSERT(result, result.GetMergedMessage());
}

template <typename RefArrayType, typename TestArrayType>
void TestValues(const RefArrayType& refArray, const TestArrayType& testArray)
{
  TestValuesImpl(
    MakeComparable(refArray, typename vtkm::cont::internal::ArrayHandleCheck<RefArrayType>::type{}),
    MakeComparable(testArray,
                   typename vtkm::cont::internal::ArrayHandleCheck<TestArrayType>::type{}));
}

template <typename ValueType>
vtkm::cont::ArrayHandle<ValueType> MakeInputArray()
{
  vtkm::cont::ArrayHandle<ValueType> input;
  input.Allocate(ARRAY_SIZE);
  SetPortal(input.WritePortal());
  return input;
}

template <typename ValueType>
void TryCopy()
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             "Trying type: " << vtkm::testing::TypeName<ValueType>::Name());
  using VTraits = vtkm::VecTraits<ValueType>;

  {
    std::cout << "implicit -> basic" << std::endl;
    vtkm::cont::ArrayHandleIndex input(ARRAY_SIZE);
    vtkm::cont::ArrayHandle<typename VTraits::BaseComponentType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "basic -> basic" << std::endl;
    using SourceType = typename VTraits::template ReplaceComponentType<vtkm::Id>;
    vtkm::cont::ArrayHandle<SourceType> input = MakeInputArray<SourceType>();
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);

    output.ReleaseResources();
    vtkm::cont::ArrayCopy(vtkm::cont::UnknownArrayHandle(input), output);
    TestValues(input, output);
  }

  {
    std::cout << "implicit -> implicit (index)" << std::endl;
    vtkm::cont::ArrayHandleIndex input(ARRAY_SIZE);
    vtkm::cont::ArrayHandleIndex output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "implicit -> implicit (constant)" << std::endl;
    vtkm::cont::ArrayHandleConstant<int> input(41, ARRAY_SIZE);
    vtkm::cont::ArrayHandleConstant<int> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "implicit -> implicit (base->derived, constant)" << std::endl;
    vtkm::cont::ArrayHandle<int, vtkm::cont::StorageTagConstant> input =
      vtkm::cont::make_ArrayHandleConstant<int>(41, ARRAY_SIZE);
    vtkm::cont::ArrayHandleConstant<int> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "constant -> basic" << std::endl;
    vtkm::cont::ArrayHandleConstant<ValueType> input(TestValue(2, ValueType{}), ARRAY_SIZE);
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "counting -> basic" << std::endl;
    vtkm::cont::ArrayHandleCounting<ValueType> input(ValueType(-4), ValueType(3), ARRAY_SIZE);
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "view -> basic" << std::endl;
    vtkm::cont::ArrayHandle<ValueType> input = MakeInputArray<ValueType>();
    vtkm::cont::make_ArrayHandleView(input, 1, ARRAY_SIZE / 2);
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "concatinate -> basic" << std::endl;
    vtkm::cont::ArrayHandle<ValueType> input1 = MakeInputArray<ValueType>();
    vtkm::cont::ArrayHandleConstant<ValueType> input2(TestValue(6, ValueType{}), ARRAY_SIZE / 2);
    auto concatInput = vtkm::cont::make_ArrayHandleConcatenate(input1, input2);
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(concatInput, output);
    TestValues(concatInput, output);
  }

  {
    std::cout << "permutation -> basic" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Id> indices;
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, 2, ARRAY_SIZE / 2),
                          indices);
    auto input = vtkm::cont::make_ArrayHandlePermutation(indices, MakeInputArray<ValueType>());
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "unknown -> unknown" << std::endl;
    vtkm::cont::UnknownArrayHandle input = MakeInputArray<ValueType>();
    vtkm::cont::UnknownArrayHandle output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "unknown -> basic (same type)" << std::endl;
    vtkm::cont::UnknownArrayHandle input = MakeInputArray<ValueType>();
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "unknown -> basic (different type)" << std::endl;
    using SourceType = typename VTraits::template ReplaceComponentType<vtkm::UInt8>;
    vtkm::cont::UnknownArrayHandle input = MakeInputArray<SourceType>();
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "unknown -> basic (different type, unsupported device)" << std::endl;
    // Force the source to be on the Serial device. If the --vtkm-device argument was
    // given with a different device (which is how ctest is set up if compiled with
    // any device), then Serial will be turned off.
    using SourceType = typename VTraits::template ReplaceComponentType<vtkm::UInt8>;
    auto rawInput = MakeInputArray<SourceType>();
    {
      // Force moving the data to the Serial device.
      vtkm::cont::Token token;
      rawInput.PrepareForInput(vtkm::cont::DeviceAdapterTagSerial{}, token);
    }
    vtkm::cont::UnknownArrayHandle input = rawInput;
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  // Test the copy methods in UnknownArrayHandle. Although this would be appropriate in
  // UnitTestUnknownArrayHandle, it is easier to test copies here.
  {
    std::cout << "unknown.DeepCopyFrom(same type)" << std::endl;
    vtkm::cont::ArrayHandle<ValueType> input = MakeInputArray<ValueType>();
    vtkm::cont::ArrayHandle<ValueType> outputArray;
    vtkm::cont::UnknownArrayHandle(outputArray).DeepCopyFrom(input);
    // Should be different arrays with same content.
    VTKM_TEST_ASSERT(input != outputArray);
    TestValues(input, outputArray);

    vtkm::cont::UnknownArrayHandle outputUnknown;
    outputUnknown.DeepCopyFrom(input);
    // Should be different arrays with same content.
    VTKM_TEST_ASSERT(input != outputUnknown.AsArrayHandle<vtkm::cont::ArrayHandle<ValueType>>());
    TestValues(input, outputUnknown);
  }

  {
    std::cout << "unknown.DeepCopyFrom(different type)" << std::endl;
    using SourceType = typename VTraits::template ReplaceComponentType<vtkm::UInt8>;
    vtkm::cont::ArrayHandle<SourceType> input = MakeInputArray<SourceType>();
    vtkm::cont::ArrayHandle<ValueType> outputArray;
    vtkm::cont::UnknownArrayHandle(outputArray).DeepCopyFrom(input);
    TestValues(input, outputArray);

    outputArray.ReleaseResources();
    vtkm::cont::UnknownArrayHandle outputUnknown(outputArray);
    outputUnknown.DeepCopyFrom(input);
    TestValues(input, outputUnknown);
  }

  {
    std::cout << "unknown.CopyShallowIfPossible(same type)" << std::endl;
    vtkm::cont::ArrayHandle<ValueType> input = MakeInputArray<ValueType>();
    vtkm::cont::UnknownArrayHandle outputUnknown;
    outputUnknown.CopyShallowIfPossible(input);
    VTKM_TEST_ASSERT(input == outputUnknown.AsArrayHandle<vtkm::cont::ArrayHandle<ValueType>>());

    vtkm::cont::ArrayHandle<ValueType> outputArray;
    outputUnknown = outputArray;
    outputUnknown.CopyShallowIfPossible(input);
    outputUnknown.AsArrayHandle(outputArray);
    VTKM_TEST_ASSERT(input == outputArray);
  }

  {
    std::cout << "unknown.CopyShallowIfPossible(different type)" << std::endl;
    using SourceType = typename VTraits::template ReplaceComponentType<vtkm::UInt8>;
    vtkm::cont::ArrayHandle<SourceType> input = MakeInputArray<SourceType>();
    vtkm::cont::ArrayHandle<ValueType> outputArray;
    vtkm::cont::UnknownArrayHandle(outputArray).CopyShallowIfPossible(input);
    TestValues(input, outputArray);

    outputArray.ReleaseResources();
    vtkm::cont::UnknownArrayHandle outputUnknown(outputArray);
    outputUnknown.CopyShallowIfPossible(input);
    TestValues(input, outputUnknown);
  }
}

void TryArrayCopyShallowIfPossible()
{
  vtkm::cont::ArrayHandle<vtkm::Float32> input = MakeInputArray<vtkm::Float32>();
  vtkm::cont::UnknownArrayHandle unknownInput = input;

  {
    std::cout << "shallow copy" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Float32> output;
    vtkm::cont::ArrayCopyShallowIfPossible(unknownInput, output);
    VTKM_TEST_ASSERT(input == output, "Copy was not shallow");
  }

  {
    std::cout << "cannot shallow copy" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Float64> output;
    vtkm::cont::ArrayCopyShallowIfPossible(unknownInput, output);
    TestValues(input, output);
  }
}

void TestArrayCopy()
{
  TryCopy<vtkm::Id>();
  TryCopy<vtkm::IdComponent>();
  TryCopy<vtkm::Float32>();
  TryCopy<vtkm::Vec3f>();
  TryCopy<vtkm::Vec4i_16>();
  TryArrayCopyShallowIfPossible();
}

} // anonymous namespace

int UnitTestArrayCopy(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayCopy, argc, argv);
}
