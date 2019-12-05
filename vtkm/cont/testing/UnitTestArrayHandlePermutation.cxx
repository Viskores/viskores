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
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/StorageBasic.h>

#include <vtkm/exec/FunctorBase.h>

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

template <typename PermutedPortalType>
struct CheckPermutationFunctor : vtkm::exec::FunctorBase
{
  PermutedPortalType PermutedPortal;

  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    using T = typename PermutedPortalType::ValueType;
    T value = this->PermutedPortal.Get(index);

    vtkm::Id permutedIndex = 2 * index;
    T expectedValue = TestValue(permutedIndex, T());

    if (!test_equal(value, expectedValue))
    {
      this->RaiseError("Encountered bad transformed value.");
    }
  }
};

template <typename PermutedArrayHandleType, typename Device>
VTKM_CONT CheckPermutationFunctor<
  typename PermutedArrayHandleType::template ExecutionTypes<Device>::PortalConst>
make_CheckPermutationFunctor(const PermutedArrayHandleType& permutedArray, Device)
{
  using PermutedPortalType =
    typename PermutedArrayHandleType::template ExecutionTypes<Device>::PortalConst;
  CheckPermutationFunctor<PermutedPortalType> functor;
  functor.PermutedPortal = permutedArray.PrepareForInput(Device());
  return functor;
}

template <typename PermutedPortalType>
struct InPlacePermutationFunctor : vtkm::exec::FunctorBase
{
  PermutedPortalType PermutedPortal;

  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    using T = typename PermutedPortalType::ValueType;
    T value = this->PermutedPortal.Get(index);

    value = value + T(1000);

    this->PermutedPortal.Set(index, value);
  }
};

template <typename PermutedArrayHandleType, typename Device>
VTKM_CONT InPlacePermutationFunctor<
  typename PermutedArrayHandleType::template ExecutionTypes<Device>::Portal>
make_InPlacePermutationFunctor(PermutedArrayHandleType& permutedArray, Device)
{
  using PermutedPortalType =
    typename PermutedArrayHandleType::template ExecutionTypes<Device>::Portal;
  InPlacePermutationFunctor<PermutedPortalType> functor;
  functor.PermutedPortal = permutedArray.PrepareForInPlace(Device());
  return functor;
}

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

template <typename PermutedPortalType>
struct OutputPermutationFunctor : vtkm::exec::FunctorBase
{
  PermutedPortalType PermutedPortal;

  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    using T = typename PermutedPortalType::ValueType;
    this->PermutedPortal.Set(index, TestValue(static_cast<vtkm::Id>(index), T()));
  }
};

template <typename PermutedArrayHandleType, typename Device>
VTKM_CONT OutputPermutationFunctor<
  typename PermutedArrayHandleType::template ExecutionTypes<Device>::Portal>
make_OutputPermutationFunctor(PermutedArrayHandleType& permutedArray, Device)
{
  using PermutedPortalType =
    typename PermutedArrayHandleType::template ExecutionTypes<Device>::Portal;
  OutputPermutationFunctor<PermutedPortalType> functor;
  functor.PermutedPortal = permutedArray.PrepareForOutput(ARRAY_SIZE, Device());
  return functor;
}

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

  using Device = vtkm::cont::DeviceAdapterTagSerial;
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

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
    ValueArrayType array = vtkm::cont::make_ArrayHandle(buffer);

    // Copy the array so that the data is not destroyed when we return from
    // this method.
    ValueArrayType arrayCopy;
    Algorithm::Copy(array, arrayCopy);

    return arrayCopy;
  }

  void operator()() const
  {
    std::cout << "Create ArrayHandlePermutation" << std::endl;
    IndexArrayType indexArray(DoubleIndexFunctor(), ARRAY_SIZE);

    ValueArrayType valueArray = this->MakeValueArray();

    PermutationArrayType permutationArray(indexArray, valueArray);

    VTKM_TEST_ASSERT(permutationArray.GetNumberOfValues() == ARRAY_SIZE,
                     "Permutation array wrong size.");
    VTKM_TEST_ASSERT(permutationArray.GetPortalControl().GetNumberOfValues() == ARRAY_SIZE,
                     "Permutation portal wrong size.");
    VTKM_TEST_ASSERT(permutationArray.GetPortalConstControl().GetNumberOfValues() == ARRAY_SIZE,
                     "Permutation portal wrong size.");

    std::cout << "Test initial values in execution environment" << std::endl;
    Algorithm::Schedule(make_CheckPermutationFunctor(permutationArray, Device()), ARRAY_SIZE);

    std::cout << "Try in place operation" << std::endl;
    Algorithm::Schedule(make_InPlacePermutationFunctor(permutationArray, Device()), ARRAY_SIZE);
    CheckInPlaceResult(valueArray.GetPortalControl());
    CheckInPlaceResult(valueArray.GetPortalConstControl());

    std::cout << "Try output operation" << std::endl;
    Algorithm::Schedule(make_OutputPermutationFunctor(permutationArray, Device()), ARRAY_SIZE);
    CheckOutputResult(valueArray.GetPortalConstControl());
    CheckOutputResult(valueArray.GetPortalControl());
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
