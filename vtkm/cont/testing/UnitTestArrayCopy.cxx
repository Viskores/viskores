//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleIndex.h>

#include <vtkm/TypeTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename RefPortalType, typename TestPortalType>
void TestValues(const RefPortalType& refPortal, const TestPortalType& testPortal)
{
  const vtkm::Id arraySize = refPortal.GetNumberOfValues();
  VTKM_TEST_ASSERT(arraySize == testPortal.GetNumberOfValues(), "Wrong array size.");

  for (vtkm::Id index = 0; index < arraySize; ++index)
  {
    VTKM_TEST_ASSERT(test_equal(refPortal.Get(index), testPortal.Get(index)), "Got bad value.");
  }
}

template <typename ValueType>
void TryCopy()
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             "Trying type: " << vtkm::testing::TypeName<ValueType>::Name());

  { // implicit -> basic
    vtkm::cont::ArrayHandleIndex input(ARRAY_SIZE);
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input.GetPortalConstControl(), output.GetPortalConstControl());
  }

  { // basic -> basic
    vtkm::cont::ArrayHandleIndex source(ARRAY_SIZE);
    vtkm::cont::ArrayHandle<vtkm::Id> input;
    vtkm::cont::ArrayCopy(source, input);
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input.GetPortalConstControl(), output.GetPortalConstControl());
  }

  { // implicit -> implicit (index)
    vtkm::cont::ArrayHandleIndex input(ARRAY_SIZE);
    vtkm::cont::ArrayHandleIndex output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input.GetPortalConstControl(), output.GetPortalConstControl());
  }

  { // implicit -> implicit (constant)
    vtkm::cont::ArrayHandleConstant<int> input(41, ARRAY_SIZE);
    vtkm::cont::ArrayHandleConstant<int> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input.GetPortalConstControl(), output.GetPortalConstControl());
  }

  { // implicit -> implicit (base->derived, constant)
    vtkm::cont::ArrayHandle<int, vtkm::cont::StorageTagConstant> input =
      vtkm::cont::make_ArrayHandleConstant<int>(41, ARRAY_SIZE);
    vtkm::cont::ArrayHandleConstant<int> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input.GetPortalConstControl(), output.GetPortalConstControl());
  }
}

void TestArrayCopy()
{
  TryCopy<vtkm::Id>();
  TryCopy<vtkm::IdComponent>();
  TryCopy<vtkm::Float32>();
}

} // anonymous namespace

int UnitTestArrayCopy(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayCopy, argc, argv);
}
