//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/VecTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

const vtkm::Id ARRAY_SIZE = 10;

template <typename ValueType>
struct IndexSquared
{
  VTKM_EXEC_CONT
  ValueType operator()(vtkm::Id i) const
  {
    using ComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;
    return ValueType(static_cast<ComponentType>(i * i));
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

struct ImplicitTests
{
  template <typename ValueType>
  void operator()(const ValueType) const
  {
    using FunctorType = IndexSquared<ValueType>;
    FunctorType functor;

    using ImplicitHandle = vtkm::cont::ArrayHandleImplicit<FunctorType>;

    ImplicitHandle implicit = vtkm::cont::make_ArrayHandleImplicit(functor, ARRAY_SIZE);

    std::cout << "verify that the control portal works" << std::endl;
    auto implicitPortal = implicit.ReadPortal();
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const ValueType v = implicitPortal.Get(i);
      const ValueType correct_value = functor(i);
      VTKM_TEST_ASSERT(v == correct_value, "Implicit Handle Failed");
    }

    std::cout << "verify that the execution portal works" << std::endl;
    vtkm::cont::Token token;
    using Device = vtkm::cont::DeviceAdapterTagSerial;
    using CEPortal = typename ImplicitHandle::ReadPortalType;
    CEPortal execPortal = implicit.PrepareForInput(Device(), token);
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const ValueType v = execPortal.Get(i);
      const ValueType correct_value = functor(i);
      VTKM_TEST_ASSERT(v == correct_value, "Implicit Handle Failed");
    }

    std::cout << "verify that the array handle works in a worklet on the device" << std::endl;
    vtkm::cont::Invoker invoke;
    vtkm::cont::ArrayHandle<ValueType> result;
    invoke(PassThrough{}, implicit, result);
    auto resultPortal = result.ReadPortal();
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const ValueType value = resultPortal.Get(i);
      const ValueType correctValue = functor(i);
      VTKM_TEST_ASSERT(test_equal(value, correctValue));
    }
  }
};

void TestArrayHandleImplicit()
{
  vtkm::testing::Testing::TryTypes(ImplicitTests(), vtkm::TypeListCommon());
}

} // anonymous namespace

int UnitTestArrayHandleImplicit(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleImplicit, argc, argv);
}
