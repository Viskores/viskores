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

#include <vtkm/cont/ArrayHandle2D.h>

#include <vtkm/cont/testing/Testing.h>

namespace UnitTestArrayHandle2DNamespace {

const vtkm::Id N = 10;

void TestArrayHandle2D()
{
  typedef vtkm::cont::ArrayHandle2D<vtkm::Float64> Array2D;
  Array2D array;
  array.Allocate( N );
 
  VTKM_TEST_ASSERT(array.GetNumberOfValues() == N, "Bad size.");

  for (vtkm::Id index = 0; index < N; index++)
  {
    std::cout << array.GetPortalConstControl().Get(index) << std::endl;
  }
}

} 

int UnitTestArrayHandle2D(int, char *[])
{
  using namespace UnitTestArrayHandle2DNamespace;
  return vtkm::cont::testing::Testing::Run(TestArrayHandle2D);
}
