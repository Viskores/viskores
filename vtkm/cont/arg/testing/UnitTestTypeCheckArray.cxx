//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/arg/TypeCheckTagArrayIn.h>
#include <vtkm/cont/arg/TypeCheckTagArrayInOut.h>
#include <vtkm/cont/arg/TypeCheckTagArrayOut.h>
#include <vtkm/cont/arg/TypeCheckTagAtomicArray.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleCounting.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

struct TryArraysOfType
{
  template <typename T>
  void operator()(T) const
  {
    using vtkm::cont::arg::TypeCheck;
    using vtkm::cont::arg::TypeCheckTagArrayIn;
    using vtkm::cont::arg::TypeCheckTagArrayInOut;
    using vtkm::cont::arg::TypeCheckTagArrayOut;

    using StandardArray = vtkm::cont::ArrayHandle<T>;
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArrayIn, StandardArray>::value),
                     "Standard array type check failed.");
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArrayInOut, StandardArray>::value),
                     "Standard array type check failed.");
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArrayOut, StandardArray>::value),
                     "Standard array type check failed.");

    using CountingArray = vtkm::cont::ArrayHandleCounting<T>;
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArrayIn, CountingArray>::value),
                     "Counting array type check failed.");
    VTKM_TEST_ASSERT((!TypeCheck<TypeCheckTagArrayInOut, CountingArray>::value),
                     "Counting array type check failed.");
    VTKM_TEST_ASSERT((!TypeCheck<TypeCheckTagArrayOut, CountingArray>::value),
                     "Counting array type check failed.");

    using CompositeArray = vtkm::cont::ArrayHandleCompositeVector<StandardArray, StandardArray>;
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArrayIn, CompositeArray>::value),
                     "Composite array type check failed.");
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArrayInOut, CompositeArray>::value),
                     "Counting array type check failed.");
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArrayOut, CompositeArray>::value),
                     "Counting array type check failed.");

    // Just some type that is not a valid array.
    using NotAnArray = typename StandardArray::WritePortalType;
    VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArrayIn, NotAnArray>::value),
                     "Not an array type check failed.");
    VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArrayInOut, NotAnArray>::value),
                     "Not an array type check failed.");
    VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArrayOut, NotAnArray>::value),
                     "Not an array type check failed.");

    // Another type that is not a valid array.
    VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArrayIn, T>::value),
                     "Not an array type check failed.");
    VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArrayInOut, T>::value),
                     "Not an array type check failed.");
    VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArrayOut, T>::value),
                     "Not an array type check failed.");
  }
};

void TestCheckAtomicArray()
{
  std::cout << "Trying some arrays with atomic arrays." << std::endl;
  using vtkm::cont::arg::TypeCheck;
  using vtkm::cont::arg::TypeCheckTagAtomicArray;

  using Int32Array = vtkm::cont::ArrayHandle<vtkm::Int32>;
  using Int64Array = vtkm::cont::ArrayHandle<vtkm::Int64>;
  using FloatArray = vtkm::cont::ArrayHandle<vtkm::Float32>;

  VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagAtomicArray, Int32Array>::value),
                   "Check for 32-bit int failed.");
  VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagAtomicArray, Int64Array>::value),
                   "Check for 64-bit int failed.");
  VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagAtomicArray, FloatArray>::value),
                   "Check for float failed.");
}

void TestCheckArray()
{
  vtkm::testing::Testing::TryTypes(TryArraysOfType());

  TestCheckAtomicArray();
}

} // anonymous namespace

int UnitTestTypeCheckArray(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCheckArray, argc, argv);
}
