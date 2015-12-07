//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
  typedef void ControlSignature(WholeArrayIn<>,
                                WholeArrayInOut<>,
                                WholeArrayOut<>);
  typedef void ExecutionSignature(WorkIndex, _1, _2, _3);

  template<typename InPortalType,
           typename InOutPortalType,
           typename OutPortalType>
  VTKM_EXEC_EXPORT
  void operator()(const vtkm::Id &index,
                  const InPortalType &inPortal,
                  const InOutPortalType &inOutPortal,
                  const OutPortalType &outPortal) const
  {
    typedef typename InPortalType::ValueType inT;
    if (!test_equal(inPortal.Get(index), TestValue(index, inT())))
    {
      this->RaiseError("Got wrong input value.");
    }

    typedef typename InOutPortalType::ValueType inOutT;
    if (!test_equal(inOutPortal.Get(index),
                    TestValue(index, inOutT()) + inOutT(100)))
    {
      this->RaiseError("Got wrong input/output value.");
    }
    inOutPortal.Set(index, TestValue(index, inOutT()));

    typedef typename OutPortalType::ValueType outT;
    outPortal.Set(index, TestValue(index, outT()));
  }
};

namespace map_whole_array {

static const vtkm::Id ARRAY_SIZE = 10;

template<typename WorkletType>
struct DoTestWorklet
{
  // This just demonstrates that the WholeArray tags support dynamic arrays.
  VTKM_CONT_EXPORT
  void CallWorklet(const vtkm::cont::DynamicArrayHandle &inArray,
                   const vtkm::cont::DynamicArrayHandle &inOutArray,
                   const vtkm::cont::DynamicArrayHandle &outArray) const
  {
    std::cout << "Create and run dispatcher." << std::endl;
    vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(inArray, inOutArray, outArray);
  }

  template<typename T>
  VTKM_CONT_EXPORT
  void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inArray[ARRAY_SIZE];
    T inOutArray[ARRAY_SIZE];

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inArray[index] = TestValue(index, T());
      inOutArray[index] = TestValue(index, T()) + T(100);
    }

    vtkm::cont::ArrayHandle<T> inHandle =
        vtkm::cont::make_ArrayHandle(inArray, ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> inOutHandle =
        vtkm::cont::make_ArrayHandle(inOutArray, ARRAY_SIZE);
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

void TestWorkletMapFieldExecArg()
{
  typedef vtkm::cont::internal::DeviceAdapterTraits<
                    VTKM_DEFAULT_DEVICE_ADAPTER_TAG> DeviceAdapterTraits;
  std::cout << "Testing Worklet with WholeArray on device adapter: "
            << DeviceAdapterTraits::GetId() << std::endl;

  std::cout << "--- Worklet accepting all types." << std::endl;
  vtkm::testing::Testing::TryTypes(
                         map_whole_array::DoTestWorklet< TestWholeArrayWorklet >(),
                         vtkm::TypeListTagCommon());

}

} // anonymous namespace

int UnitTestWorkletMapFieldWholeArray(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(map_whole_array::TestWorkletMapFieldExecArg);
}
