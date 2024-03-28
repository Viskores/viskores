//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleGroupVecVariable.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

// GroupVecVariable is a bit strange because it supports values of different
// lengths, so a simple pass through worklet will not work. Use custom
// worklets.
struct GroupVariableInputWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, WorkIndex, _2);

  template <typename InputType>
  VTKM_EXEC void operator()(const InputType& input, vtkm::Id workIndex, vtkm::Id& dummyOut) const
  {
    using ComponentType = typename InputType::ComponentType;
    vtkm::IdComponent expectedSize = static_cast<vtkm::IdComponent>(workIndex);
    if (expectedSize != input.GetNumberOfComponents())
    {
      this->RaiseError("Got unexpected number of components.");
    }

    vtkm::Id valueIndex = workIndex * (workIndex - 1) / 2;
    dummyOut = valueIndex;
    for (vtkm::IdComponent componentIndex = 0; componentIndex < expectedSize; componentIndex++)
    {
      ComponentType expectedValue = TestValue(valueIndex, ComponentType());
      if (vtkm::Abs(expectedValue - input[componentIndex]) > 0.000001)
      {
        this->RaiseError("Got bad value in GroupVariableInputWorklet.");
      }
      valueIndex++;
    }
  }
};

struct TestGroupVecVariableAsInput
{
  template <typename ComponentType>
  VTKM_CONT void operator()(ComponentType) const
  {
    vtkm::cont::Invoker invoke;
    vtkm::Id sourceArraySize;

    vtkm::cont::ArrayHandle<vtkm::Id> numComponentsArray;
    vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(ARRAY_SIZE), numComponentsArray);
    vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray =
      vtkm::cont::ConvertNumComponentsToOffsets(numComponentsArray, sourceArraySize);

    vtkm::cont::ArrayHandle<ComponentType> sourceArray;
    sourceArray.Allocate(sourceArraySize);
    SetPortal(sourceArray.WritePortal());

    vtkm::cont::ArrayHandle<vtkm::Id> dummyArray;

    auto groupVecArray = vtkm::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray);

    VTKM_TEST_ASSERT(groupVecArray.GetNumberOfValues() == ARRAY_SIZE);
    // Num components is inconsistent, so you should just get 0.
    VTKM_TEST_ASSERT(groupVecArray.GetNumberOfComponentsFlat() == 0);

    invoke(GroupVariableInputWorklet{}, groupVecArray, dummyArray);

    dummyArray.ReadPortal();

    groupVecArray.ReleaseResources();
  }
};

// GroupVecVariable is a bit strange because it supports values of different
// lengths, so a simple pass through worklet will not work. Use custom
// worklets.
struct GroupVariableOutputWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_2, WorkIndex);

  template <typename OutputType>
  VTKM_EXEC void operator()(OutputType& output, vtkm::Id workIndex) const
  {
    using ComponentType = typename OutputType::ComponentType;
    vtkm::IdComponent expectedSize = static_cast<vtkm::IdComponent>(workIndex);
    if (expectedSize != output.GetNumberOfComponents())
    {
      this->RaiseError("Got unexpected number of components.");
    }

    vtkm::Id valueIndex = workIndex * (workIndex - 1) / 2;
    for (vtkm::IdComponent componentIndex = 0; componentIndex < expectedSize; componentIndex++)
    {
      output[componentIndex] = TestValue(valueIndex, ComponentType());
      valueIndex++;
    }
  }
};

struct TestGroupVecVariableAsOutput
{
  template <typename ComponentType>
  VTKM_CONT void operator()(ComponentType) const
  {
    vtkm::Id sourceArraySize;

    vtkm::cont::ArrayHandle<vtkm::Id> numComponentsArray;
    vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(ARRAY_SIZE), numComponentsArray);
    vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray =
      vtkm::cont::ConvertNumComponentsToOffsets(numComponentsArray, sourceArraySize);

    vtkm::cont::ArrayHandle<ComponentType> sourceArray;
    sourceArray.Allocate(sourceArraySize);

    vtkm::worklet::DispatcherMapField<GroupVariableOutputWorklet> dispatcher;
    dispatcher.Invoke(vtkm::cont::ArrayHandleIndex(ARRAY_SIZE),
                      vtkm::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray));

    CheckPortal(sourceArray.ReadPortal());
  }
};

void Run()
{
  using ScalarTypesToTest = vtkm::List<vtkm::UInt8, vtkm::FloatDefault>;

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleGroupVecVariable as Input" << std::endl;
  vtkm::testing::Testing::TryTypes(TestGroupVecVariableAsInput(), ScalarTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleGroupVecVariable as Output" << std::endl;
  vtkm::testing::Testing::TryTypes(TestGroupVecVariableAsOutput(), ScalarTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleGroupVecVariable(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
