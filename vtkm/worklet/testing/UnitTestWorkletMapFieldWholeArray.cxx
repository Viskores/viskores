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
#include <vtkm/cont/VariantArrayHandle.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

class TestWholeArrayWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, WholeArrayInOut, WholeArrayOut);
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

namespace map_whole_array
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

struct DoTestWholeArrayWorklet
{
  using WorkletType = TestWholeArrayWorklet;

  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inArray[ARRAY_SIZE];
    T inOutArray[ARRAY_SIZE];

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inArray[index] = TestValue(index, T());
      inOutArray[index] = static_cast<T>(TestValue(index, T()) + T(100));
    }

    vtkm::cont::ArrayHandle<T> inHandle = vtkm::cont::make_ArrayHandle(inArray, ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> inOutHandle = vtkm::cont::make_ArrayHandle(inOutArray, ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> outHandle;
    // Output arrays must be preallocated.
    outHandle.Allocate(ARRAY_SIZE);

    vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(vtkm::cont::VariantArrayHandle(inHandle).ResetTypes(vtkm::List<T>{}),
                      vtkm::cont::VariantArrayHandle(inOutHandle).ResetTypes(vtkm::List<T>{}),
                      vtkm::cont::VariantArrayHandle(outHandle).ResetTypes(vtkm::List<T>{}));

    std::cout << "Check result." << std::endl;
    CheckPortal(inOutHandle.GetPortalConstControl());
    CheckPortal(outHandle.GetPortalConstControl());
  }
};

void TestWorkletMapFieldExecArg(vtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Worklet with WholeArray on device adapter: " << id.GetName() << std::endl;

  std::cout << "--- Worklet accepting all types." << std::endl;
  vtkm::testing::Testing::TryTypes(map_whole_array::DoTestWholeArrayWorklet(),
                                   vtkm::TypeListCommon());
}

} // anonymous namespace

int UnitTestWorkletMapFieldWholeArray(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::RunOnDevice(
    map_whole_array::TestWorkletMapFieldExecArg, argc, argv);
}
