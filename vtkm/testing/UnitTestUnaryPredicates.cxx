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

#include <vtkm/UnaryPredicates.h>

#include <vtkm/testing/Testing.h>

namespace
{

template <typename T>
void UnaryPredicateTest(){ //test IsZeroInitialized
                           { vtkm::IsZeroInitialized is_default;
VTKM_TEST_ASSERT(is_default(vtkm::TypeTraits<T>::ZeroInitialization()) == true,
                 "IsZeroInitialized wrong.");
VTKM_TEST_ASSERT(is_default(TestValue(1, T())) == false, "IsZeroInitialized wrong.");
}

//test NotZeroInitialized
{
  vtkm::NotZeroInitialized not_default;
  VTKM_TEST_ASSERT(not_default(vtkm::TypeTraits<T>::ZeroInitialization()) == false,
                   "NotZeroInitialized wrong.");
  VTKM_TEST_ASSERT(not_default(TestValue(1, T())) == true, "NotZeroInitialized wrong.");
}
}
;

struct UnaryPredicateTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    UnaryPredicateTest<T>();
  }
};

void TestUnaryPredicates()
{
  vtkm::testing::Testing::TryTypes(UnaryPredicateTestFunctor());

  //test LogicalNot
  {
    vtkm::LogicalNot logical_not;
    VTKM_TEST_ASSERT(logical_not(true) == false, "logical_not true wrong.");
    VTKM_TEST_ASSERT(logical_not(false) == true, "logical_not false wrong.");
  }
}

} // anonymous namespace

int UnitTestUnaryPredicates(int, char* [])
{
  return vtkm::testing::Testing::Run(TestUnaryPredicates);
}
