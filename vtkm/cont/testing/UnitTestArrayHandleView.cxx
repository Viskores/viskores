//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleView.h>

#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename ValueType>
struct IndexSquared
{
  VTKM_EXEC_CONT
  ValueType operator()(vtkm::Id index) const
  {
    using ComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;
    return ValueType(static_cast<ComponentType>(index * index));
  }
};

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

struct TestViewAsInput
{
  template <typename ValueType>
  VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
  {
    using FunctorType = IndexSquared<ValueType>;

    using ValueHandleType = vtkm::cont::ArrayHandleImplicit<FunctorType>;
    using ViewHandleType = vtkm::cont::ArrayHandleView<ValueHandleType>;

    FunctorType functor;
    for (vtkm::Id start_pos = 0; start_pos < ARRAY_SIZE; start_pos += ARRAY_SIZE / 4)
    {
      const vtkm::Id counting_ARRAY_SIZE = ARRAY_SIZE - start_pos;

      ValueHandleType implicit = vtkm::cont::make_ArrayHandleImplicit(functor, ARRAY_SIZE);

      ViewHandleType view =
        vtkm::cont::make_ArrayHandleView(implicit, start_pos, counting_ARRAY_SIZE);
      VTKM_TEST_ASSERT(view.GetNumberOfComponentsFlat() ==
                       vtkm::VecFlat<ValueType>::NUM_COMPONENTS);
      VTKM_TEST_ASSERT(view.GetNumberOfValues() == counting_ARRAY_SIZE);

      vtkm::cont::ArrayHandle<ValueType> result;

      vtkm::cont::Invoker invoke;
      invoke(PassThrough{}, view, result);

      //verify that the control portal works
      auto resultPortal = result.ReadPortal();
      auto implicitPortal = implicit.ReadPortal();
      auto viewPortal = view.ReadPortal();
      for (vtkm::Id i = 0; i < counting_ARRAY_SIZE; ++i)
      {
        const vtkm::Id value_index = i;
        const vtkm::Id key_index = start_pos + i;

        const ValueType result_v = resultPortal.Get(value_index);
        const ValueType correct_value = implicitPortal.Get(key_index);
        const ValueType control_value = viewPortal.Get(value_index);
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Implicit Handle Failed");
        VTKM_TEST_ASSERT(test_equal(result_v, control_value), "Implicit Handle Failed");
      }

      view.ReleaseResources();
    }
  }
};

struct TestViewAsOutput
{
  template <typename ValueType>
  VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
  {
    using ValueHandleType = vtkm::cont::ArrayHandle<ValueType>;
    using ViewHandleType = vtkm::cont::ArrayHandleView<ValueHandleType>;

    vtkm::cont::ArrayHandle<ValueType> input;
    input.Allocate(ARRAY_SIZE);
    SetPortal(input.WritePortal());

    ValueHandleType values;
    values.Allocate(ARRAY_SIZE * 2);

    ViewHandleType view = vtkm::cont::make_ArrayHandleView(values, ARRAY_SIZE, ARRAY_SIZE);
    vtkm::cont::Invoker invoke;
    invoke(PassThrough{}, input, view);

    //verify that the control portal works
    CheckPortal(view.ReadPortal());

    //verify that filling works
    const ValueType expected = TestValue(20, ValueType{});
    view.Fill(expected);
    auto valuesPortal = values.ReadPortal();
    for (vtkm::Id index = ARRAY_SIZE; index < 2 * ARRAY_SIZE; ++index)
    {
      VTKM_TEST_ASSERT(valuesPortal.Get(index) == expected);
    }
  }
};

void Run()
{
  using HandleTypesToTest =
    vtkm::List<vtkm::Id, vtkm::Vec2i_32, vtkm::FloatDefault, vtkm::Vec3f_64>;

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleView as Input" << std::endl;
  vtkm::testing::Testing::TryTypes(TestViewAsInput(), HandleTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleView as Output" << std::endl;
  vtkm::testing::Testing::TryTypes(TestViewAsOutput(), HandleTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleView(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
