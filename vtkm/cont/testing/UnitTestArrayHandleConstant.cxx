//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleConstant.h>

#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

using HandleTypesToTest = vtkm::List<vtkm::Id, vtkm::Vec2i_32, vtkm::FloatDefault, vtkm::Vec3f_64>;

struct PassThrough : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename InValue, typename OutValue>
  VTKM_EXEC void operator()(const InValue& inValue, OutValue& outValue) const
  {
    outValue = inValue;
  }
};

struct TestConstantAsInput
{
  template <typename ValueType>
  VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
  {
    const ValueType value = TestValue(43, ValueType());

    vtkm::cont::ArrayHandleConstant<ValueType> constant =
      vtkm::cont::make_ArrayHandleConstant(value, ARRAY_SIZE);

    VTKM_TEST_ASSERT(constant.GetValue() == value);
    VTKM_TEST_ASSERT(constant.GetNumberOfValues() == ARRAY_SIZE);
    VTKM_TEST_ASSERT(constant.GetNumberOfComponentsFlat() ==
                     vtkm::VecFlat<ValueType>::NUM_COMPONENTS);

    vtkm::cont::ArrayHandle<ValueType> result;

    vtkm::cont::Invoker invoke;
    invoke(PassThrough{}, constant, result);

    //verify that the control portal works
    auto resultPortal = result.ReadPortal();
    auto constantPortal = constant.ReadPortal();
    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      const ValueType result_v = resultPortal.Get(i);
      const ValueType control_value = constantPortal.Get(i);
      VTKM_TEST_ASSERT(test_equal(result_v, value), "Counting Handle Failed");
      VTKM_TEST_ASSERT(test_equal(result_v, control_value), "Counting Handle Control Failed");
    }

    constant.ReleaseResources();
  }
};

void Run()
{
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleConstant as Input" << std::endl;
  vtkm::testing::Testing::TryTypes(TestConstantAsInput(), HandleTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleConstant(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
