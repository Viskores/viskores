//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/DynamicArrayHandle.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/exec/ExecutionWholeArray.h>

#include <vtkm/cont/testing/Testing.h>

class TestExecObjectWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<>, ExecObject, ExecObject, FieldOut<>);
  typedef void ExecutionSignature(_1, _2, _3, _4);

  template <typename T, typename StorageTag>
  VTKM_EXEC void operator()(const vtkm::Id& index,
                            const vtkm::exec::ExecutionWholeArrayConst<T, StorageTag>& execIn,
                            vtkm::exec::ExecutionWholeArray<T, StorageTag>& execOut,
                            T& out) const
  {
    if (!test_equal(execIn.Get(index), TestValue(index, T()) + T(100)))
    {
      this->RaiseError("Got wrong input value.");
    }
    out = execIn.Get(index) - T(100);
    execOut.Set(index, out);
  }

  template <typename T1, typename T2, typename T3>
  VTKM_EXEC void operator()(const vtkm::Id&, const T1&, const T2&, const T3&) const
  {
    this->RaiseError("Cannot call this worklet with different types.");
  }
};

namespace map_exec_field
{

static const vtkm::Id ARRAY_SIZE = 10;

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
      inputArray[index] = TestValue(index, T()) + T(100);
    }

    vtkm::cont::ArrayHandleIndex counting(ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> inputHandle = vtkm::cont::make_ArrayHandle(inputArray, ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> outputHandle;
    vtkm::cont::ArrayHandle<T> outputFieldArray;

    std::cout << "Create and run dispatcher." << std::endl;
    vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(counting,
                      vtkm::exec::ExecutionWholeArrayConst<T>(inputHandle),
                      vtkm::exec::ExecutionWholeArray<T>(outputHandle, ARRAY_SIZE),
                      outputFieldArray);

    std::cout << "Check result." << std::endl;
    CheckPortal(outputHandle.GetPortalConstControl());
    CheckPortal(outputFieldArray.GetPortalConstControl());

    std::cout << "Repeat with dynamic arrays." << std::endl;
    // Clear out output arrays.
    outputFieldArray = vtkm::cont::ArrayHandle<T>();
    outputHandle = vtkm::cont::ArrayHandle<T>();

    vtkm::cont::DynamicArrayHandle outputFieldDynamic(outputFieldArray);
    dispatcher.Invoke(counting,
                      vtkm::exec::ExecutionWholeArrayConst<T>(inputHandle),
                      vtkm::exec::ExecutionWholeArray<T>(outputHandle, ARRAY_SIZE),
                      outputFieldDynamic);

    std::cout << "Check dynamic array result." << std::endl;
    CheckPortal(outputHandle.GetPortalConstControl());
    CheckPortal(outputFieldArray.GetPortalConstControl());
  }
};

void TestWorkletMapFieldExecArg()
{
  typedef vtkm::cont::DeviceAdapterTraits<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> DeviceAdapterTraits;
  std::cout << "Testing Worklet with ExecutionWholeArray on device adapter: "
            << DeviceAdapterTraits::GetName() << std::endl;

  std::cout << "--- Worklet accepting all types." << std::endl;
  vtkm::testing::Testing::TryTypes(map_exec_field::DoTestWorklet<TestExecObjectWorklet>(),
                                   vtkm::TypeListTagCommon());
}

} // anonymous namespace

int UnitTestWorkletMapFieldExecArg(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(map_exec_field::TestWorkletMapFieldExecArg);
}
