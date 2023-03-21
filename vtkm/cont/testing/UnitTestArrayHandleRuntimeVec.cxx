//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleRuntimeVec.h>

#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

struct UnusualType
{
  vtkm::Id X;
};

} // anonymous namespace

namespace detail
{

template <>
struct TestValueImpl<UnusualType>
{
  VTKM_EXEC_CONT UnusualType operator()(vtkm::Id index) const
  {
    return { TestValue(index, decltype(UnusualType::X){}) };
  }
};

template <>
struct TestEqualImpl<UnusualType, UnusualType>
{
  VTKM_EXEC_CONT bool operator()(UnusualType value1,
                                 UnusualType value2,
                                 vtkm::Float64 tolerance) const
  {
    return test_equal(value1.X, value2.X, tolerance);
  }
};

} // namespace detail

namespace
{

struct PassThrough : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename InValue, typename OutValue>
  VTKM_EXEC void operator()(const InValue& inValue, OutValue& outValue) const
  {
    vtkm::IdComponent inIndex = 0;
    vtkm::IdComponent outIndex = 0;
    this->FlatCopy(inValue, inIndex, outValue, outIndex);
  }

  template <typename InValue, typename OutValue>
  VTKM_EXEC void FlatCopy(const InValue& inValue,
                          vtkm::IdComponent& inIndex,
                          OutValue& outValue,
                          vtkm::IdComponent& outIndex) const
  {
    using VTraitsIn = vtkm::VecTraits<InValue>;
    using VTraitsOut = vtkm::VecTraits<OutValue>;
    VTraitsOut::SetComponent(outValue, outIndex, VTraitsIn::GetComponent(inValue, inIndex));
    inIndex++;
    outIndex++;
  }

  template <typename InComponent, vtkm::IdComponent InN, typename OutValue>
  VTKM_EXEC void FlatCopy(const vtkm::Vec<InComponent, InN>& inValue,
                          vtkm::IdComponent& inIndex,
                          OutValue& outValue,
                          vtkm::IdComponent& outIndex) const
  {
    VTKM_ASSERT(inIndex == 0);
    for (vtkm::IdComponent i = 0; i < InN; ++i)
    {
      FlatCopy(inValue[i], inIndex, outValue, outIndex);
      inIndex = 0;
    }
  }

  template <typename InValue, typename OutComponent, vtkm::IdComponent OutN>
  VTKM_EXEC void FlatCopy(const InValue& inValue,
                          vtkm::IdComponent& inIndex,
                          vtkm::Vec<OutComponent, OutN>& outValue,
                          vtkm::IdComponent& outIndex) const
  {
    VTKM_ASSERT(outIndex == 0);
    for (vtkm::IdComponent i = 0; i < OutN; ++i)
    {
      OutComponent outComponent;
      FlatCopy(inValue, inIndex, outComponent, outIndex);
      outValue[i] = outComponent;
      outIndex = 0;
    }
  }
};

template <vtkm::IdComponent NUM_COMPONENTS>
struct TestRuntimeVecAsInput
{
  template <typename ComponentType>
  VTKM_CONT void operator()(ComponentType) const
  {
    using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;

    vtkm::cont::ArrayHandle<ComponentType> baseArray;
    baseArray.Allocate(ARRAY_SIZE * NUM_COMPONENTS);
    SetPortal(baseArray.WritePortal());

    auto runtimeVecArray = vtkm::cont::make_ArrayHandleRuntimeVec(NUM_COMPONENTS, baseArray);
    VTKM_TEST_ASSERT(runtimeVecArray.GetNumberOfValues() == ARRAY_SIZE,
                     "Group array reporting wrong array size.");

    vtkm::cont::ArrayHandle<ValueType> resultArray;

    vtkm::cont::Invoker{}(PassThrough{}, runtimeVecArray, resultArray);

    VTKM_TEST_ASSERT(resultArray.GetNumberOfValues() == ARRAY_SIZE, "Got bad result array size.");

    //verify that the control portal works
    vtkm::Id totalIndex = 0;
    auto resultPortal = resultArray.ReadPortal();
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      const ValueType result = resultPortal.Get(index);
      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
      {
        const ComponentType expectedValue = TestValue(totalIndex, ComponentType());
        VTKM_TEST_ASSERT(test_equal(result[componentIndex], expectedValue),
                         "Result array got wrong value.");
        totalIndex++;
      }
    }

    //verify that you can get the data as a basic array
    vtkm::cont::ArrayHandle<vtkm::Vec<ComponentType, NUM_COMPONENTS>> flatComponents;
    runtimeVecArray.AsArrayHandleBasic(flatComponents);
    VTKM_TEST_ASSERT(test_equal_ArrayHandles(
      flatComponents, vtkm::cont::make_ArrayHandleGroupVec<NUM_COMPONENTS>(baseArray)));

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec<ComponentType, 1>, NUM_COMPONENTS>>
      nestedComponents;
    runtimeVecArray.AsArrayHandleBasic(nestedComponents);
    auto flatPortal = flatComponents.ReadPortal();
    auto nestedPortal = nestedComponents.ReadPortal();
    for (vtkm::Id index = 0; index < flatPortal.GetNumberOfValues(); ++index)
    {
      VTKM_TEST_ASSERT(test_equal(vtkm::make_VecFlat(flatPortal.Get(index)),
                                  vtkm::make_VecFlat(nestedPortal.Get(index))));
    }

    runtimeVecArray.ReleaseResources();
  }
};

template <vtkm::IdComponent NUM_COMPONENTS>
struct TestRuntimeVecAsOutput
{
  template <typename ComponentType>
  VTKM_CONT void operator()(ComponentType) const
  {
    using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;

    vtkm::cont::ArrayHandle<ValueType> baseArray;
    baseArray.Allocate(ARRAY_SIZE);
    SetPortal(baseArray.WritePortal());

    vtkm::cont::ArrayHandle<ComponentType> resultArray;

    vtkm::cont::ArrayHandleRuntimeVec<ComponentType> runtimeVecArray(NUM_COMPONENTS, resultArray);

    vtkm::cont::Invoker{}(PassThrough{}, baseArray, runtimeVecArray);

    VTKM_TEST_ASSERT(runtimeVecArray.GetNumberOfValues() == ARRAY_SIZE,
                     "Group array reporting wrong array size.");

    VTKM_TEST_ASSERT(resultArray.GetNumberOfValues() == ARRAY_SIZE * NUM_COMPONENTS,
                     "Got bad result array size.");

    //verify that the control portal works
    vtkm::Id totalIndex = 0;
    auto resultPortal = resultArray.ReadPortal();
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      const ValueType expectedValue = TestValue(index, ValueType());
      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
      {
        const ComponentType result = resultPortal.Get(totalIndex);
        VTKM_TEST_ASSERT(test_equal(result, expectedValue[componentIndex]),
                         "Result array got wrong value.");
        totalIndex++;
      }
    }
  }
};

void Run()
{
  using HandleTypesToTest =
    vtkm::List<vtkm::Id, vtkm::Vec2i_32, vtkm::FloatDefault, vtkm::Vec3f_64>;
  using ScalarTypesToTest = vtkm::List<vtkm::UInt8, vtkm::FloatDefault>;

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRuntimeVec(3) as Input" << std::endl;
  vtkm::testing::Testing::TryTypes(TestRuntimeVecAsInput<3>(), HandleTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRuntimeVec(4) as Input" << std::endl;
  vtkm::testing::Testing::TryTypes(TestRuntimeVecAsInput<4>(), HandleTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRuntimeVec(2) as Output" << std::endl;
  vtkm::testing::Testing::TryTypes(TestRuntimeVecAsOutput<2>(), ScalarTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRuntimeVec(3) as Output" << std::endl;
  vtkm::testing::Testing::TryTypes(TestRuntimeVecAsOutput<3>(), ScalarTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRuntimeVec(3) as Input with unusual type" << std::endl;
  TestRuntimeVecAsInput<3>{}(UnusualType{});
}

} // anonymous namespace

int UnitTestArrayHandleRuntimeVec(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
