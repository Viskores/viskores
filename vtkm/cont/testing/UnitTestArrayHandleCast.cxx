//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleCast.h>

#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

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

struct TestCastAsInput
{
  template <typename CastToType>
  VTKM_CONT void operator()(CastToType vtkmNotUsed(type)) const
  {
    vtkm::cont::Invoker invoke;
    using InputArrayType = vtkm::cont::ArrayHandleIndex;

    InputArrayType input(ARRAY_SIZE);
    vtkm::cont::ArrayHandleCast<CastToType, InputArrayType> castArray =
      vtkm::cont::make_ArrayHandleCast(input, CastToType());
    vtkm::cont::ArrayHandle<CastToType> result;

    invoke(PassThrough{}, castArray, result);

    // verify results
    vtkm::Id length = ARRAY_SIZE;
    auto resultPortal = result.ReadPortal();
    auto inputPortal = input.ReadPortal();
    for (vtkm::Id i = 0; i < length; ++i)
    {
      VTKM_TEST_ASSERT(resultPortal.Get(i) == static_cast<CastToType>(inputPortal.Get(i)),
                       "Casting ArrayHandle Failed");
    }

    castArray.ReleaseResources();
  }
};

struct TestCastAsOutput
{
  template <typename CastFromType>
  VTKM_CONT void operator()(CastFromType vtkmNotUsed(type)) const
  {
    vtkm::cont::Invoker invoke;

    using InputArrayType = vtkm::cont::ArrayHandleIndex;
    using ResultArrayType = vtkm::cont::ArrayHandle<CastFromType>;

    InputArrayType input(ARRAY_SIZE);

    ResultArrayType result;
    vtkm::cont::ArrayHandleCast<vtkm::Id, ResultArrayType> castArray =
      vtkm::cont::make_ArrayHandleCast<CastFromType>(result);

    invoke(PassThrough{}, input, castArray);

    // verify results
    vtkm::Id length = ARRAY_SIZE;
    auto inputPortal = input.ReadPortal();
    auto resultPortal = result.ReadPortal();
    for (vtkm::Id i = 0; i < length; ++i)
    {
      VTKM_TEST_ASSERT(inputPortal.Get(i) == static_cast<vtkm::Id>(resultPortal.Get(i)),
                       "Casting ArrayHandle Failed");
    }
  }
};

void Run()
{
  using CastTypesToTest = vtkm::List<vtkm::Int32, vtkm::UInt32>;

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleCast as Input" << std::endl;
  vtkm::testing::Testing::TryTypes(TestCastAsInput(), CastTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleCast as Output" << std::endl;
  vtkm::testing::Testing::TryTypes(TestCastAsOutput(), CastTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleCast(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
