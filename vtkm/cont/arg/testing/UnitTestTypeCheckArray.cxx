//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/arg/TypeCheckTagArray.h>
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
    using TypeCheckTagArray = vtkm::cont::arg::TypeCheckTagArray<vtkm::TypeListTagAll>;

    using StandardArray = vtkm::cont::ArrayHandle<T>;
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArray, StandardArray>::value),
                     "Standard array type check failed.");

    using CountingArray = vtkm::cont::ArrayHandleCounting<T>;
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArray, CountingArray>::value),
                     "Counting array type check failed.");

    using CompositeArray = vtkm::cont::ArrayHandleCompositeVector<StandardArray, CountingArray>;
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArray, CompositeArray>::value),
                     "Composite array type check failed.");

    // Just some type that is not a valid array.
    using NotAnArray = typename StandardArray::PortalControl;
    VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArray, NotAnArray>::value),
                     "Not an array type check failed.");

    // Another type that is not a valid array.
    VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArray, T>::value), "Not an array type check failed.");
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

  using DefaultTypeCheck = TypeCheckTagAtomicArray<>;
  VTKM_TEST_ASSERT((TypeCheck<DefaultTypeCheck, Int32Array>::value),
                   "Check for 32-bit int failed.");
  VTKM_TEST_ASSERT((TypeCheck<DefaultTypeCheck, Int64Array>::value),
                   "Check for 64-bit int failed.");
  VTKM_TEST_ASSERT(!(TypeCheck<DefaultTypeCheck, FloatArray>::value), "Check for float failed.");

  using ExpandedTypeCheck = TypeCheckTagAtomicArray<vtkm::TypeListTagAll>;
  VTKM_TEST_ASSERT((TypeCheck<ExpandedTypeCheck, Int32Array>::value),
                   "Check for 32-bit int failed.");
  VTKM_TEST_ASSERT((TypeCheck<ExpandedTypeCheck, Int64Array>::value),
                   "Check for 64-bit int failed.");
  VTKM_TEST_ASSERT(!(TypeCheck<ExpandedTypeCheck, FloatArray>::value), "Check for float failed.");

  using RestrictedTypeCheck = TypeCheckTagAtomicArray<vtkm::ListTagBase<vtkm::Int32>>;
  VTKM_TEST_ASSERT((TypeCheck<RestrictedTypeCheck, Int32Array>::value),
                   "Check for 32-bit int failed.");
  VTKM_TEST_ASSERT(!(TypeCheck<RestrictedTypeCheck, Int64Array>::value),
                   "Check for 64-bit int failed.");
  VTKM_TEST_ASSERT(!(TypeCheck<RestrictedTypeCheck, FloatArray>::value), "Check for float failed.");
}

void TestCheckArray()
{
  vtkm::testing::Testing::TryTypes(TryArraysOfType());

  std::cout << "Trying some arrays with types that do not match the list." << std::endl;
  using vtkm::cont::arg::TypeCheck;
  using vtkm::cont::arg::TypeCheckTagArray;

  using ScalarArray = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArray<vtkm::TypeListTagFieldScalar>, ScalarArray>::value),
                   "Scalar for scalar check failed.");
  VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArray<vtkm::TypeListTagFieldVec3>, ScalarArray>::value),
                   "Scalar for vector check failed.");

  using VecArray = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>;
  VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArray<vtkm::TypeListTagFieldVec3>, VecArray>::value),
                   "Vector for vector check failed.");
  VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArray<vtkm::TypeListTagFieldScalar>, VecArray>::value),
                   "Vector for scalar check failed.");

  TestCheckAtomicArray();
}

} // anonymous namespace

int UnitTestTypeCheckArray(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestCheckArray);
}
