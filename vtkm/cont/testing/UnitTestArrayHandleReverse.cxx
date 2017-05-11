//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/ArrayHandleReverse.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/ArrayHandleIndex.h>

namespace UnitTestArrayHandleIndexNamespace {

const vtkm::Id ARRAY_SIZE = 10;

void TestArrayHandleReverse() {
  vtkm::cont::ArrayHandleIndex array(ARRAY_SIZE);
  VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Bad size.");

  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++) {
    VTKM_TEST_ASSERT(array.GetPortalConstControl().Get(index) == index,
                     "Index array has unexpected value.");
  }

  vtkm::cont::ArrayHandleReverse<vtkm::cont::ArrayHandleIndex> reverse =
    vtkm::cont::make_ArrayHandleReverse(array);

  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++) {
    VTKM_TEST_ASSERT(reverse.GetPortalConstControl().Get(index) == array.GetPortalConstControl().Get(9 - index),
                     "ArrayHandleReverse does not reverse array");
  }

}
};// namespace UnitTestArrayHandleIndexNamespace

int UnitTestArrayHandleReverse(int, char *[]) {
  using namespace UnitTestArrayHandleIndexNamespace;
  return vtkm::cont::testing::Testing::Run(TestArrayHandleReverse);
}


