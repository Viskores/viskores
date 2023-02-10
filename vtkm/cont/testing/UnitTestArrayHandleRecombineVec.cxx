//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleRecombineVec.h>

#include <vtkm/cont/ArrayHandleReverse.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

struct PassThrough : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename InValue, typename OutValue>
  VTKM_EXEC void operator()(const InValue& inValue, OutValue& outValue) const
  {
    outValue = inValue;
  }
};

struct TestRecombineVecAsInput
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    vtkm::cont::ArrayHandle<T> baseArray;
    baseArray.Allocate(ARRAY_SIZE);
    SetPortal(baseArray.WritePortal());

    using VTraits = vtkm::VecTraits<T>;
    vtkm::cont::ArrayHandleRecombineVec<typename VTraits::ComponentType> recombinedArray;
    for (vtkm::IdComponent cIndex = 0; cIndex < VTraits::NUM_COMPONENTS; ++cIndex)
    {
      recombinedArray.AppendComponentArray(vtkm::cont::ArrayExtractComponent(baseArray, cIndex));
    }
    VTKM_TEST_ASSERT(recombinedArray.GetNumberOfComponents() == VTraits::NUM_COMPONENTS);
    VTKM_TEST_ASSERT(recombinedArray.GetNumberOfValues() == ARRAY_SIZE);

    vtkm::cont::ArrayHandle<T> outputArray;
    vtkm::cont::Invoker invoke;
    invoke(PassThrough{}, recombinedArray, outputArray);

    VTKM_TEST_ASSERT(test_equal_ArrayHandles(baseArray, outputArray));
  }
};

struct TestRecombineVecAsOutput
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    vtkm::cont::ArrayHandle<T> baseArray;
    baseArray.Allocate(ARRAY_SIZE);
    SetPortal(baseArray.WritePortal());

    vtkm::cont::ArrayHandle<T> outputArray;

    using VTraits = vtkm::VecTraits<T>;
    vtkm::cont::ArrayHandleRecombineVec<typename VTraits::ComponentType> recombinedArray;
    for (vtkm::IdComponent cIndex = 0; cIndex < VTraits::NUM_COMPONENTS; ++cIndex)
    {
      recombinedArray.AppendComponentArray(vtkm::cont::ArrayExtractComponent(outputArray, cIndex));
    }
    VTKM_TEST_ASSERT(recombinedArray.GetNumberOfComponents() == VTraits::NUM_COMPONENTS);

    vtkm::cont::Invoker invoke;
    invoke(PassThrough{}, baseArray, recombinedArray);
    VTKM_TEST_ASSERT(test_equal_ArrayHandles(baseArray, outputArray));

    // Try outputing to a recombine vec inside of another fancy ArrayHandle.
    auto reverseOutput = vtkm::cont::make_ArrayHandleReverse(recombinedArray);
    invoke(PassThrough{}, baseArray, reverseOutput);
    VTKM_TEST_ASSERT(test_equal_ArrayHandles(baseArray, reverseOutput));
  }
};

void Run()
{
  using HandleTypesToTest =
    vtkm::List<vtkm::Id, vtkm::Vec2i_32, vtkm::FloatDefault, vtkm::Vec3f_64>;

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRecombineVec as Input" << std::endl;
  vtkm::testing::Testing::TryTypes(TestRecombineVecAsInput(), HandleTypesToTest{});

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRecombineVec as Output" << std::endl;
  vtkm::testing::Testing::TryTypes(TestRecombineVecAsOutput(), HandleTypesToTest{});
}

} // anonymous namespace

int UnitTestArrayHandleRecombineVec(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
