//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
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

#include <vtkm/cont/testing/Testing.h>

class TestWholeArrayWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn<>, WholeArrayInOut<>, WholeArrayOut<>);
  using ExecutionSignature = void(WorkIndex, _1, _2, _3);

  template <typename InPortalType, typename InOutPortalType, typename OutPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& index,
                            const InPortalType& inPortal,
                            const InOutPortalType& inOutPortal,
                            const OutPortalType& outPortal) const
  {
    using inT = typename InPortalType::ValueType;
    if (!test_equal(inPortal.Get(index), TestValue(index, inT())))
    {
      this->RaiseError("Got wrong input value.");
    }

    using inOutT = typename InOutPortalType::ValueType;
    if (!test_equal(inOutPortal.Get(index), TestValue(index, inOutT()) + inOutT(100)))
    {
      this->RaiseError("Got wrong input/output value.");
    }
    inOutPortal.Set(index, TestValue(index, inOutT()));

    using outT = typename OutPortalType::ValueType;
    outPortal.Set(index, TestValue(index, outT()));
  }
};

class TestAtomicArrayWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn<>, AtomicArrayInOut<>);
  using ExecutionSignature = void(WorkIndex, _2);
  using InputDomain = _1;

  template <typename AtomicArrayType>
  VTKM_EXEC void operator()(const vtkm::Id& index, const AtomicArrayType& atomicArray) const
  {
    using ValueType = typename AtomicArrayType::ValueType;
    atomicArray.Add(0, static_cast<ValueType>(index));
  }
};

namespace map_whole_array
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

struct DoTestWholeArrayWorklet
{
  using WorkletType = TestWholeArrayWorklet;

  // This just demonstrates that the WholeArray tags support dynamic arrays.
  VTKM_CONT
  void CallWorklet(const vtkm::cont::DynamicArrayHandle& inArray,
                   const vtkm::cont::DynamicArrayHandle& inOutArray,
                   const vtkm::cont::DynamicArrayHandle& outArray) const
  {
    std::cout << "Create and run dispatcher." << std::endl;
    vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(inArray, inOutArray, outArray);
  }

  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inArray[ARRAY_SIZE];
    T inOutArray[ARRAY_SIZE];

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inArray[index] = TestValue(index, T());
      inOutArray[index] = TestValue(index, T()) + T(100);
    }

    vtkm::cont::ArrayHandle<T> inHandle = vtkm::cont::make_ArrayHandle(inArray, ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> inOutHandle = vtkm::cont::make_ArrayHandle(inOutArray, ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> outHandle;
    // Output arrays must be preallocated.
    outHandle.Allocate(ARRAY_SIZE);

    this->CallWorklet(vtkm::cont::DynamicArrayHandle(inHandle),
                      vtkm::cont::DynamicArrayHandle(inOutHandle),
                      vtkm::cont::DynamicArrayHandle(outHandle));

    std::cout << "Check result." << std::endl;
    CheckPortal(inOutHandle.GetPortalConstControl());
    CheckPortal(outHandle.GetPortalConstControl());
  }
};

struct DoTestAtomicArrayWorklet
{
  using WorkletType = TestAtomicArrayWorklet;

  // This just demonstrates that the WholeArray tags support dynamic arrays.
  VTKM_CONT
  void CallWorklet(const vtkm::cont::DynamicArrayHandle& inOutArray) const
  {
    std::cout << "Create and run dispatcher." << std::endl;
    vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(vtkm::cont::ArrayHandleIndex(ARRAY_SIZE), inOutArray);
  }

  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inOutValue = 0;

    vtkm::cont::ArrayHandle<T> inOutHandle = vtkm::cont::make_ArrayHandle(&inOutValue, 1);

    this->CallWorklet(vtkm::cont::DynamicArrayHandle(inOutHandle));

    std::cout << "Check result." << std::endl;
    T result = inOutHandle.GetPortalConstControl().Get(0);

    VTKM_TEST_ASSERT(result == (ARRAY_SIZE * (ARRAY_SIZE - 1)) / 2,
                     "Got wrong summation in atomic array.");
  }
};

void TestWorkletMapFieldExecArg()
{
  using DeviceAdapterTraits = vtkm::cont::DeviceAdapterTraits<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>;
  std::cout << "Testing Worklet with WholeArray on device adapter: "
            << DeviceAdapterTraits::GetName() << std::endl;

  std::cout << "--- Worklet accepting all types." << std::endl;
  vtkm::testing::Testing::TryTypes(map_whole_array::DoTestWholeArrayWorklet(),
                                   vtkm::TypeListTagCommon());

  std::cout << "--- Worklet accepting atomics." << std::endl;
  vtkm::testing::Testing::TryTypes(map_whole_array::DoTestAtomicArrayWorklet(),
                                   vtkm::cont::AtomicArrayTypeListTag());
}

} // anonymous namespace

int UnitTestWorkletMapFieldWholeArray(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(map_whole_array::TestWorkletMapFieldExecArg);
}
