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

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>

#include <vtkm/exec/FunctorBase.h>

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

template <typename OriginalPortalType, typename TransformedPortalType>
struct CheckTransformFunctor : vtkm::exec::FunctorBase
{
  OriginalPortalType OriginalPortal;
  TransformedPortalType TransformedPortal;

  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    using T = typename TransformedPortalType::ValueType;
    typename OriginalPortalType::ValueType original = this->OriginalPortal.Get(index);
    T transformed = this->TransformedPortal.Get(index);
    if (!test_equal(transformed, MySquare{}(original)))
    {
      this->RaiseError("Encountered bad transformed value.");
    }
  }
};

template <typename OriginalArrayHandleType, typename TransformedArrayHandleType, typename Device>
VTKM_CONT CheckTransformFunctor<
  typename OriginalArrayHandleType::template ExecutionTypes<Device>::PortalConst,
  typename TransformedArrayHandleType::template ExecutionTypes<Device>::PortalConst>
make_CheckTransformFunctor(const OriginalArrayHandleType& originalArray,
                           const TransformedArrayHandleType& transformedArray,
                           Device)
{
  using OriginalPortalType =
    typename OriginalArrayHandleType::template ExecutionTypes<Device>::PortalConst;
  using TransformedPortalType =
    typename TransformedArrayHandleType::template ExecutionTypes<Device>::PortalConst;
  CheckTransformFunctor<OriginalPortalType, TransformedPortalType> functor;
  functor.OriginalPortal = originalArray.PrepareForInput(Device());
  functor.TransformedPortal = transformedArray.PrepareForInput(Device());
  return functor;
}

template <typename OriginalArrayHandleType, typename TransformedArrayHandleType>
VTKM_CONT void CheckControlPortals(const OriginalArrayHandleType& originalArray,
                                   const TransformedArrayHandleType& transformedArray)
{
  std::cout << "  Verify that the control portal works" << std::endl;

  using OriginalPortalType = typename OriginalArrayHandleType::PortalConstControl;
  using TransformedPortalType = typename TransformedArrayHandleType::PortalConstControl;

  VTKM_TEST_ASSERT(originalArray.GetNumberOfValues() == transformedArray.GetNumberOfValues(),
                   "Number of values in transformed array incorrect.");

  OriginalPortalType originalPortal = originalArray.GetPortalConstControl();
  TransformedPortalType transformedPortal = transformedArray.GetPortalConstControl();

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

    std::cout << "Test a transform handle with a counting handle as the values" << std::endl;
    vtkm::cont::ArrayHandleCounting<InputValueType> counting = vtkm::cont::make_ArrayHandleCounting(
      InputValueType(OutputValueType(0)), InputValueType(1), ARRAY_SIZE);
    CountingTransformHandle countingTransformed =
      vtkm::cont::make_ArrayHandleTransform(counting, functor);

    CheckControlPortals(counting, countingTransformed);

    std::cout << "  Verify that the execution portal works" << std::endl;
    Algorithm::Schedule(make_CheckTransformFunctor(counting, countingTransformed, Device()),
                        ARRAY_SIZE);

    std::cout << "Test a transform handle with a normal handle as the values" << std::endl;
    //we are going to connect the two handles up, and than fill
    //the values and make the transform sees the new values in the handle
    vtkm::cont::ArrayHandle<InputValueType> input;
    TransformHandle thandle(input, functor);

    using Portal = typename vtkm::cont::ArrayHandle<InputValueType>::PortalControl;
    input.Allocate(ARRAY_SIZE);
    Portal portal = input.GetPortalControl();
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      portal.Set(index, TestValue(index, InputValueType()));
    }

    CheckControlPortals(input, thandle);

    std::cout << "  Verify that the execution portal works" << std::endl;
    Algorithm::Schedule(make_CheckTransformFunctor(input, thandle, Device()), ARRAY_SIZE);

    std::cout << "Modify array handle values to ensure transform gets updated" << std::endl;
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      portal.Set(index, TestValue(index * index, InputValueType()));
    }

    CheckControlPortals(input, thandle);

    std::cout << "  Verify that the execution portal works" << std::endl;
    Algorithm::Schedule(make_CheckTransformFunctor(input, thandle, Device()), ARRAY_SIZE);
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
