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

class TestAtomicArrayWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
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

struct DoTestAtomicArrayWorklet
{
  using WorkletType = TestAtomicArrayWorklet;

  // This just demonstrates that the WholeArray tags support dynamic arrays.
  VTKM_CONT
  void CallWorklet(const vtkm::cont::VariantArrayHandle& inOutArray) const
  {
    std::cout << "Create and run dispatcher." << std::endl;
    vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(vtkm::cont::ArrayHandleIndex(ARRAY_SIZE),
                      inOutArray.ResetTypes<vtkm::cont::AtomicArrayTypeList>());
  }

  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inOutValue = 0;

    vtkm::cont::ArrayHandle<T> inOutHandle = vtkm::cont::make_ArrayHandle(&inOutValue, 1);

    this->CallWorklet(vtkm::cont::VariantArrayHandle(inOutHandle));

    std::cout << "Check result." << std::endl;
    T result = inOutHandle.GetPortalConstControl().Get(0);

    VTKM_TEST_ASSERT(result == (ARRAY_SIZE * (ARRAY_SIZE - 1)) / 2,
                     "Got wrong summation in atomic array.");
  }
};

void TestWorkletMapFieldExecArgAtomic(vtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Worklet with AtomicWholeArray on device adapter: " << id.GetName()
            << std::endl;
  vtkm::testing::Testing::TryTypes(map_whole_array::DoTestAtomicArrayWorklet(),
                                   vtkm::cont::AtomicArrayTypeList());
}

} // anonymous namespace

int UnitTestWorkletMapFieldWholeArrayAtomic(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::RunOnDevice(
    map_whole_array::TestWorkletMapFieldExecArgAtomic, argc, argv);
}
