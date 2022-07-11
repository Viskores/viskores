//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleGroupVec.h>

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

template <vtkm::IdComponent NUM_COMPONENTS>
struct TestGroupVecAsInput
{
  template <typename ComponentType>
  VTKM_CONT void operator()(ComponentType) const
  {
    using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;

    vtkm::cont::ArrayHandle<ComponentType> baseArray;
    baseArray.Allocate(ARRAY_SIZE * NUM_COMPONENTS);
    SetPortal(baseArray.WritePortal());

    vtkm::cont::ArrayHandleGroupVec<vtkm::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS>
      groupArray(baseArray);
    VTKM_TEST_ASSERT(groupArray.GetNumberOfValues() == ARRAY_SIZE,
                     "Group array reporting wrong array size.");

    vtkm::cont::ArrayHandle<ValueType> resultArray;

    vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
    dispatcher.Invoke(groupArray, resultArray);

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

    groupArray.ReleaseResources();
  }
};

template <vtkm::IdComponent NUM_COMPONENTS>
struct TestGroupVecAsOutput
{
  template <typename ComponentType>
  VTKM_CONT void operator()(ComponentType) const
  {
    using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;

    vtkm::cont::ArrayHandle<ValueType> baseArray;
    baseArray.Allocate(ARRAY_SIZE);
    SetPortal(baseArray.WritePortal());

    vtkm::cont::ArrayHandle<ComponentType> resultArray;

    vtkm::cont::ArrayHandleGroupVec<vtkm::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS>
      groupArray(resultArray);

    vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
    dispatcher.Invoke(baseArray, groupArray);

    VTKM_TEST_ASSERT(groupArray.GetNumberOfValues() == ARRAY_SIZE,
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
  std::cout << "Testing ArrayHandleGroupVec<3> as Input" << std::endl;
  vtkm::testing::Testing::TryTypes(TestGroupVecAsInput<3>(), HandleTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleGroupVec<4> as Input" << std::endl;
  vtkm::testing::Testing::TryTypes(TestGroupVecAsInput<4>(), HandleTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleGroupVec<2> as Output" << std::endl;
  vtkm::testing::Testing::TryTypes(TestGroupVecAsOutput<2>(), ScalarTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleGroupVec<3> as Output" << std::endl;
  vtkm::testing::Testing::TryTypes(TestGroupVecAsOutput<3>(), ScalarTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleGroupVec(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
