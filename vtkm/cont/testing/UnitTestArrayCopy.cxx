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
#include <vtkm/cont/UncertainArrayHandle.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/TypeTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename RefArrayType, typename TestArrayType>
void TestValues(const RefArrayType& refArray, const TestArrayType& testArray)
{
  auto result = test_equal_ArrayHandles(refArray, testArray);
  VTKM_TEST_ASSERT(result, result.GetMergedMessage());
}

template <typename ValueType>
vtkm::cont::ArrayHandle<ValueType> MakeInputArray()
{
  vtkm::cont::ArrayHandle<ValueType> input;
  input.Allocate(ARRAY_SIZE);
  SetPortal(input.WritePortal());
  return input;
}

template <typename ValueType>
void TryCopy()
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             "Trying type: " << vtkm::testing::TypeName<ValueType>::Name());

  {
    std::cout << "implicit -> basic" << std::endl;
    vtkm::cont::ArrayHandleIndex input(ARRAY_SIZE);
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "basic -> basic" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Id> input = MakeInputArray<vtkm::Id>();
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);

    output.ReleaseResources();
    vtkm::cont::ArrayCopy(vtkm::cont::UnknownArrayHandle(input), output);
    TestValues(input, output);
  }

  {
    std::cout << "implicit -> implicit (index)" << std::endl;
    vtkm::cont::ArrayHandleIndex input(ARRAY_SIZE);
    vtkm::cont::ArrayHandleIndex output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "implicit -> implicit (constant)" << std::endl;
    vtkm::cont::ArrayHandleConstant<int> input(41, ARRAY_SIZE);
    vtkm::cont::ArrayHandleConstant<int> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "implicit -> implicit (base->derived, constant)" << std::endl;
    vtkm::cont::ArrayHandle<int, vtkm::cont::StorageTagConstant> input =
      vtkm::cont::make_ArrayHandleConstant<int>(41, ARRAY_SIZE);
    vtkm::cont::ArrayHandleConstant<int> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  using TypeList = vtkm::ListAppend<vtkm::TypeListField, vtkm::List<ValueType, vtkm::UInt8>>;
  using StorageList = VTKM_DEFAULT_STORAGE_LIST;
  using UnknownArray = vtkm::cont::UnknownArrayHandle;
  using UncertainArray = vtkm::cont::UncertainArrayHandle<TypeList, StorageList>;

  {
    std::cout << "unknown -> unknown" << std::endl;
    UnknownArray input = MakeInputArray<ValueType>();
    UnknownArray output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "uncertain -> basic (same type)" << std::endl;
    UncertainArray input = MakeInputArray<ValueType>();
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "uncertain -> basic (different type)" << std::endl;
    UncertainArray input = MakeInputArray<vtkm::UInt8>();
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }
}

void TryArrayCopyShallowIfPossible()
{
  vtkm::cont::ArrayHandle<vtkm::Float32> input = MakeInputArray<vtkm::Float32>();
  vtkm::cont::UnknownArrayHandle unknownInput = input;

  {
    std::cout << "shallow copy" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Float32> output;
    vtkm::cont::ArrayCopyShallowIfPossible(unknownInput, output);
    VTKM_TEST_ASSERT(input == output, "Copy was not shallow");
  }

  {
    std::cout << "cannot shallow copy" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Float64> output;
    vtkm::cont::ArrayCopyShallowIfPossible(unknownInput, output);
    TestValues(input, output);
  }
}

void TestArrayCopy()
{
  TryCopy<vtkm::Id>();
  TryCopy<vtkm::IdComponent>();
  TryCopy<vtkm::Float32>();
  TryArrayCopyShallowIfPossible();
}

} // anonymous namespace

int UnitTestArrayCopy(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayCopy, argc, argv);
}
