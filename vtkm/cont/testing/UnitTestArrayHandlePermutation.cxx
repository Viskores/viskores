//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandlePermutation.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace
{

const vtkm::Id ARRAY_SIZE = 10;

struct DoubleIndexFunctor
{
  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id index) const { return 2 * index; }
};

using DoubleIndexArrayType = vtkm::cont::ArrayHandleImplicit<DoubleIndexFunctor>;

struct CheckPermutationWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn permutationArray);
  using ExecutionSignature = void(WorkIndex, _1);

  template <typename T>
  VTKM_EXEC void operator()(vtkm::Id index, const T& value) const
  {
    vtkm::Id permutedIndex = 2 * index;
    T expectedValue = TestValue(permutedIndex, T());

    if (!test_equal(value, expectedValue))
    {
      this->RaiseError("Encountered bad transformed value.");
    }
  }
};

struct InPlacePermutationWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut permutationArray);

  template <typename T>
  VTKM_EXEC void operator()(T& value) const
  {
    value = value + T(1000);
  }
};

template <typename PortalType>
VTKM_CONT void CheckInPlaceResult(PortalType portal)
{
  using T = typename PortalType::ValueType;
  for (vtkm::Id permutedIndex = 0; permutedIndex < 2 * ARRAY_SIZE; permutedIndex++)
  {
    if (permutedIndex % 2 == 0)
    {
      // This index was part of the permuted array; has a value changed
      T expectedValue = TestValue(permutedIndex, T()) + T(1000);
      T retrievedValue = portal.Get(permutedIndex);
      VTKM_TEST_ASSERT(test_equal(expectedValue, retrievedValue), "Permuted set unexpected value.");
    }
    else
    {
      // This index was not part of the permuted array; has original value
      T expectedValue = TestValue(permutedIndex, T());
      T retrievedValue = portal.Get(permutedIndex);
      VTKM_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                       "Permuted array modified value it should not have.");
    }
  }
}

struct OutputPermutationWorklet : vtkm::worklet::WorkletMapField
{
  // Note: Using a FieldOut for the input domain is rare (and mostly discouraged),
  // but it works as long as the array is allocated to the size desired.
  using ControlSignature = void(FieldOut permutationArray);
  using ExecutionSignature = void(WorkIndex, _1);

  template <typename T>
  VTKM_EXEC void operator()(vtkm::Id index, T& value) const
  {
    value = TestValue(static_cast<vtkm::Id>(index), T());
  }
};

template <typename PortalType>
VTKM_CONT void CheckOutputResult(PortalType portal)
{
  using T = typename PortalType::ValueType;
  for (vtkm::IdComponent permutedIndex = 0; permutedIndex < 2 * ARRAY_SIZE; permutedIndex++)
  {
    if (permutedIndex % 2 == 0)
    {
      // This index was part of the permuted array; has a value changed
      vtkm::Id originalIndex = permutedIndex / 2;
      T expectedValue = TestValue(originalIndex, T());
      T retrievedValue = portal.Get(permutedIndex);
      VTKM_TEST_ASSERT(test_equal(expectedValue, retrievedValue), "Permuted set unexpected value.");
    }
    else
    {
      // This index was not part of the permuted array; has original value
      T expectedValue = TestValue(permutedIndex, T());
      T retrievedValue = portal.Get(permutedIndex);
      VTKM_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                       "Permuted array modified value it should not have.");
    }
  }
}

template <typename ValueType>
struct PermutationTests
{
  using IndexArrayType = vtkm::cont::ArrayHandleImplicit<DoubleIndexFunctor>;
  using ValueArrayType = vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagBasic>;
  using PermutationArrayType = vtkm::cont::ArrayHandlePermutation<IndexArrayType, ValueArrayType>;

  ValueArrayType MakeValueArray() const
  {
    // Allocate a buffer and set initial values
    std::vector<ValueType> buffer(2 * ARRAY_SIZE);
    for (vtkm::IdComponent index = 0; index < 2 * ARRAY_SIZE; index++)
    {
      vtkm::UInt32 i = static_cast<vtkm::UInt32>(index);
      buffer[i] = TestValue(index, ValueType());
    }

    // Create an ArrayHandle from the buffer
    return vtkm::cont::make_ArrayHandle(buffer, vtkm::CopyFlag::On);
  }

  void operator()() const
  {
    std::cout << "Create ArrayHandlePermutation" << std::endl;
    IndexArrayType indexArray(DoubleIndexFunctor(), ARRAY_SIZE);

    ValueArrayType valueArray = this->MakeValueArray();

    PermutationArrayType permutationArray(indexArray, valueArray);

    VTKM_TEST_ASSERT(permutationArray.GetNumberOfValues() == ARRAY_SIZE,
                     "Permutation array wrong size.");
    VTKM_TEST_ASSERT(permutationArray.WritePortal().GetNumberOfValues() == ARRAY_SIZE,
                     "Permutation portal wrong size.");
    VTKM_TEST_ASSERT(permutationArray.ReadPortal().GetNumberOfValues() == ARRAY_SIZE,
                     "Permutation portal wrong size.");

    vtkm::cont::Invoker invoke;

    std::cout << "Test initial values in execution environment" << std::endl;
    invoke(CheckPermutationWorklet{}, permutationArray);

    std::cout << "Try in place operation" << std::endl;
    invoke(InPlacePermutationWorklet{}, permutationArray);
    CheckInPlaceResult(valueArray.WritePortal());
    CheckInPlaceResult(valueArray.ReadPortal());

    std::cout << "Try output operation" << std::endl;
    invoke(OutputPermutationWorklet{}, permutationArray);
    CheckOutputResult(valueArray.ReadPortal());
    CheckOutputResult(valueArray.WritePortal());
  }
};

struct TryInputType
{
  template <typename InputType>
  void operator()(InputType) const
  {
    PermutationTests<InputType>()();
  }
};

void TestArrayHandlePermutation()
{
  vtkm::testing::Testing::TryTypes(TryInputType(), vtkm::TypeListCommon());
}

} // anonymous namespace

int UnitTestArrayHandlePermutation(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandlePermutation, argc, argv);
}
