//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleTransform.h>

#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

const vtkm::Id ARRAY_SIZE = 10;

struct MySquare
{
  template <typename U>
  VTKM_EXEC auto operator()(U u) const -> decltype(vtkm::Dot(u, u))
  {
    return vtkm::Dot(u, u);
  }
};

struct CheckTransformWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn original, FieldIn transformed);

  template <typename T, typename U>
  VTKM_EXEC void operator()(const T& original, const U& transformed) const
  {
    if (!test_equal(transformed, MySquare{}(original)))
    {
      this->RaiseError("Encountered bad transformed value.");
    }
  }
};

template <typename OriginalArrayHandleType, typename TransformedArrayHandleType>
VTKM_CONT void CheckControlPortals(const OriginalArrayHandleType& originalArray,
                                   const TransformedArrayHandleType& transformedArray)
{
  std::cout << "  Verify that the control portal works" << std::endl;

  using OriginalPortalType = typename OriginalArrayHandleType::ReadPortalType;
  using TransformedPortalType = typename TransformedArrayHandleType::ReadPortalType;

  VTKM_TEST_ASSERT(originalArray.GetNumberOfValues() == transformedArray.GetNumberOfValues(),
                   "Number of values in transformed array incorrect.");

  OriginalPortalType originalPortal = originalArray.ReadPortal();
  TransformedPortalType transformedPortal = transformedArray.ReadPortal();

  VTKM_TEST_ASSERT(originalPortal.GetNumberOfValues() == transformedPortal.GetNumberOfValues(),
                   "Number of values in transformed portal incorrect.");

  for (vtkm::Id index = 0; index < originalArray.GetNumberOfValues(); index++)
  {
    using T = typename TransformedPortalType::ValueType;
    typename OriginalPortalType::ValueType original = originalPortal.Get(index);
    T transformed = transformedPortal.Get(index);
    VTKM_TEST_ASSERT(test_equal(transformed, MySquare{}(original)), "Bad transform value.");
  }
}

template <typename InputValueType>
struct TransformTests
{
  using OutputValueType = typename vtkm::VecTraits<InputValueType>::ComponentType;

  using TransformHandle =
    vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandle<InputValueType>, MySquare>;

  using CountingTransformHandle =
    vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandleCounting<InputValueType>, MySquare>;

  using Device = vtkm::cont::DeviceAdapterTagSerial;
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

  void operator()() const
  {
    MySquare functor;
    vtkm::cont::Invoker invoke;

    std::cout << "Test a transform handle with a counting handle as the values" << std::endl;
    vtkm::cont::ArrayHandleCounting<InputValueType> counting = vtkm::cont::make_ArrayHandleCounting(
      InputValueType(OutputValueType(0)), InputValueType(1), ARRAY_SIZE);
    CountingTransformHandle countingTransformed =
      vtkm::cont::make_ArrayHandleTransform(counting, functor);

    CheckControlPortals(counting, countingTransformed);

    std::cout << "  Verify that the execution portal works" << std::endl;
    invoke(CheckTransformWorklet{}, counting, countingTransformed);

    std::cout << "Test a transform handle with a normal handle as the values" << std::endl;
    //we are going to connect the two handles up, and than fill
    //the values and make the transform sees the new values in the handle
    vtkm::cont::ArrayHandle<InputValueType> input;
    TransformHandle thandle(input, functor);

    using Portal = typename vtkm::cont::ArrayHandle<InputValueType>::WritePortalType;
    input.Allocate(ARRAY_SIZE);
    {
      Portal portal = input.WritePortal();
      for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        portal.Set(index, TestValue(index, InputValueType()));
      }
    }

    CheckControlPortals(input, thandle);

    std::cout << "  Verify that the execution portal works" << std::endl;
    invoke(CheckTransformWorklet{}, input, thandle);

    std::cout << "Modify array handle values to ensure transform gets updated" << std::endl;
    {
      Portal portal = input.WritePortal();
      for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        portal.Set(index, TestValue(index * index, InputValueType()));
      }
    }

    CheckControlPortals(input, thandle);

    std::cout << "  Verify that the execution portal works" << std::endl;
    invoke(CheckTransformWorklet{}, input, thandle);
  }
};

struct TryInputType
{
  template <typename InputType>
  void operator()(InputType) const
  {
    TransformTests<InputType>()();
  }
};

void TestArrayHandleTransform()
{
  vtkm::testing::Testing::TryTypes(TryInputType());
}

} // anonymous namespace

int UnitTestArrayHandleTransform(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleTransform, argc, argv);
}
