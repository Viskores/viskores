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

#include <vtkm/cont/ArrayHandleInterpreter.h>
#include <vtkm/cont/ArrayHandleConcatenate2DTopDown.h>

#include <vtkm/cont/testing/Testing.h>

namespace UnitTestArrayHandleInterpreterNamespace {

const vtkm::Id NX = 5;
const vtkm::Id NY = 5;
const vtkm::Id NZ = 1;
const vtkm::Id LEN = NX * NY * NZ;

void TestArrayHandleInterpreter()
{
  typedef vtkm::cont::ArrayHandleInterpreter<vtkm::Float64> ArrayInterpreter;
  ArrayInterpreter array1, array2;
  array1.Allocate( LEN );
  array2.Allocate( LEN );
  
  array1.InterpretAs2D( NX, NY );
  array2.InterpretAs2D( NX, NY );

  vtkm::Id val = 0;
/*
  for( vtkm::Id k = 0; k < NZ; k++ )
    for( vtkm::Id j = 0; j < NY; j++ )
      for( vtkm::Id i = 0; i < NX; i++ )
      {
        array1.Set2D( i, j, val++ );
        array2.Set2D( i, j, val++ );
      }
*/
        
  typedef vtkm::cont::ArrayHandleConcatenate2DTopDown
          < ArrayInterpreter, ArrayInterpreter>     Concat2DTopDown;
  Concat2DTopDown arrayTopDown( array1, array2 );

  for( vtkm::Id j = 0; j < NY*2; j++ )
    for( vtkm::Id i = 0; i < NX; i++ ) 
      arrayTopDown.Set2D( i, j, val++ );
      //std::cout << arrayTopDown.Get2D(i, j) << std::endl;

  for (vtkm::Id index = 0; index < arrayTopDown.GetNumberOfValues(); index++)
  {
    std::cout << arrayTopDown.GetPortalConstControl().Get(index) << std::endl;
  }
  
}

} 

int UnitTestArrayHandleInterpreter(int, char *[])
{
  using namespace UnitTestArrayHandleInterpreterNamespace;
  return vtkm::cont::testing::Testing::Run(TestArrayHandleInterpreter);
}
