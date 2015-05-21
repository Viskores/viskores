//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/arg/TypeCheckTagArray.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleCounting.h>

#include <vtkm/cont/testing/Testing.h>

namespace {

struct TryArraysOfType
{
  template<typename T>
  void operator()(T) const
  {
    using vtkm::cont::arg::TypeCheck;
    typedef vtkm::cont::arg::TypeCheckTagArray<vtkm::TypeListTagAll>
        TypeCheckTagArray;

    typedef vtkm::cont::ArrayHandle<T> StandardArray;
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArray,StandardArray>::value),
                     "Standard array type check failed.");

    typedef vtkm::cont::ArrayHandleCounting<T> CountingArray;
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArray,CountingArray>::value),
                     "Counting array type check failed.");

    typedef typename vtkm::cont::ArrayHandleCompositeVectorType<
        StandardArray,CountingArray>::type CompositeArray;
    VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArray,CompositeArray>::value),
                     "Composite array type check failed.");

    // Just some type that is not a valid array.
    typedef typename StandardArray::PortalControl NotAnArray;
    VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArray,NotAnArray>::value),
                     "Not an array type check failed.");

    // Another type that is not a valid array.
    VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArray,T>::value),
                     "Not an array type check failed.");
  }
};

void TestCheckArray()
{
  vtkm::testing::Testing::TryAllTypes(TryArraysOfType());

  std::cout << "Trying some arrays with types that do not match the list."
            << std::endl;
  using vtkm::cont::arg::TypeCheck;
  using vtkm::cont::arg::TypeCheckTagArray;

  typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault> ScalarArray;
  VTKM_TEST_ASSERT(
        (TypeCheck<TypeCheckTagArray<vtkm::TypeListTagFieldScalar>,ScalarArray>::value),
        "Scalar for scalar check failed.");
  VTKM_TEST_ASSERT(
        !(TypeCheck<TypeCheckTagArray<vtkm::TypeListTagFieldVec3>,ScalarArray>::value),
        "Scalar for vector check failed.");

  typedef vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault,3> > VecArray;
  VTKM_TEST_ASSERT(
        (TypeCheck<TypeCheckTagArray<vtkm::TypeListTagFieldVec3>,VecArray>::value),
        "Vector for vector check failed.");
  VTKM_TEST_ASSERT(
        !(TypeCheck<TypeCheckTagArray<vtkm::TypeListTagFieldScalar>,VecArray>::value),
        "Vector for scalar check failed.");
}

} // anonymous namespace

int UnitTestTypeCheckArray(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestCheckArray);
}
