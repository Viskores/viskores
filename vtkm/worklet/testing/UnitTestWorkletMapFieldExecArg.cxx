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
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/VariantArrayHandle.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

struct SimpleExecObject : vtkm::cont::ExecutionObjectBase
{
  template <typename Device>
  Device PrepareForExecution(Device) const
  {
    return Device();
  }
};

struct TestExecObjectWorklet
{
  template <typename T>
  class Worklet : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayOut, FieldOut, ExecObject);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, Device);

    template <typename InPortalType, typename OutPortalType, typename DeviceTag>
    VTKM_EXEC void operator()(const vtkm::Id& index,
                              const InPortalType& execIn,
                              OutPortalType& execOut,
                              T& out,
                              DeviceTag,
                              DeviceTag) const
    {
      VTKM_IS_DEVICE_ADAPTER_TAG(DeviceTag);

      if (!test_equal(execIn.Get(index), TestValue(index, T()) + T(100)))
      {
        this->RaiseError("Got wrong input value.");
      }
      out = static_cast<T>(execIn.Get(index) - T(100));
      execOut.Set(index, out);
    }
  };
};

namespace map_exec_field
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename WorkletType>
struct DoTestWorklet
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inputArray[ARRAY_SIZE];

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inputArray[index] = static_cast<T>(TestValue(index, T()) + T(100));
    }

    vtkm::cont::ArrayHandleIndex counting(ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> inputHandle = vtkm::cont::make_ArrayHandle(inputArray, ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> outputHandle;
    vtkm::cont::ArrayHandle<T> outputFieldArray;
    outputHandle.Allocate(ARRAY_SIZE);

    std::cout << "Create and run dispatcher." << std::endl;
    vtkm::worklet::DispatcherMapField<typename WorkletType::template Worklet<T>> dispatcher;
    dispatcher.Invoke(counting, inputHandle, outputHandle, outputFieldArray, SimpleExecObject());

    std::cout << "Check result." << std::endl;
    CheckPortal(outputHandle.GetPortalConstControl());
    CheckPortal(outputFieldArray.GetPortalConstControl());

    std::cout << "Repeat with dynamic arrays." << std::endl;
    // Clear out output arrays.
    outputFieldArray = vtkm::cont::ArrayHandle<T>();
    outputHandle = vtkm::cont::ArrayHandle<T>();
    outputHandle.Allocate(ARRAY_SIZE);

    vtkm::cont::VariantArrayHandleBase<vtkm::List<T>> outputFieldDynamic(outputFieldArray);
    dispatcher.Invoke(counting, inputHandle, outputHandle, outputFieldDynamic, SimpleExecObject());

    std::cout << "Check dynamic array result." << std::endl;
    CheckPortal(outputHandle.GetPortalConstControl());
    CheckPortal(outputFieldArray.GetPortalConstControl());
  }
};

void TestWorkletMapFieldExecArg(vtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Worklet with WholeArray on device adapter: " << id.GetName() << std::endl;

  std::cout << "--- Worklet accepting all types." << std::endl;
  vtkm::testing::Testing::TryTypes(map_exec_field::DoTestWorklet<TestExecObjectWorklet>(),
                                   vtkm::TypeListCommon());
}

} // anonymous namespace

int UnitTestWorkletMapFieldExecArg(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::RunOnDevice(
    map_exec_field::TestWorkletMapFieldExecArg, argc, argv);
}
