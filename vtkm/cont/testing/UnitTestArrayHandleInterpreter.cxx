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
#include <vtkm/cont/ArrayHandleConcatenate2DLeftRight.h>

#include <vtkm/cont/testing/Testing.h>

namespace UnitTestArrayHandleInterpreterNamespace {

const vtkm::Id NX = 5;
const vtkm::Id NY = 5;
const vtkm::Id NZ = 1;
const vtkm::Id LEN = NX * NY * NZ;

void TestArrayHandleInterpreter()
{
  typedef vtkm::cont::ArrayHandleInterpreter<vtkm::Id> ArrayInterpreter;
  ArrayInterpreter array1, array2;    // NX x NY
  array1.Allocate( LEN );
  array2.Allocate( LEN );
  
  array1.InterpretAs2D( NX, NY );
  array2.InterpretAs2D( NX, NY );

  vtkm::Id val = 0;
        
  typedef vtkm::cont::ArrayHandleConcatenate2DTopDown
          < ArrayInterpreter, ArrayInterpreter>     Concat2DTopDown;
  Concat2DTopDown arrayTopDown( array1, array2 );   // NX x 2NY
  for( vtkm::Id j = 0; j < arrayTopDown.GetDimY(); j++ )
    for( vtkm::Id i = 0; i < arrayTopDown.GetDimX(); i++ ) 
    {
      arrayTopDown.Set2D( i, j, val++ ); 
    }

  ArrayInterpreter array3;        // NX x 2NY
  array3.Allocate( LEN * 2 );
  val = 0;
  for( vtkm::Id i = 0; i < array3.GetNumberOfValues(); i++ )
    array3.GetPortalControl().Set(i, val++ );
  array3.InterpretAs2D( NX, NY*2 );

  typedef vtkm::cont::ArrayHandleConcatenate2DLeftRight
          < ArrayInterpreter, Concat2DTopDown >     Concat2DLeftRight;
  Concat2DLeftRight arrayLeftRight( array3, arrayTopDown ); // 2NX x 2NY

  val = 100;
  for( vtkm::Id j = 0; j < arrayLeftRight.GetDimY(); j++ )
    for( vtkm::Id i = 0; i < arrayLeftRight.GetDimX(); i++ ) 
    {
      arrayLeftRight.Set2D( i, j, val++ ); 
    }

/*
  array1.PrintInfo();
  array2.PrintInfo();
  array3.PrintInfo();
  arrayTopDown.PrintInfo();
  arrayLeftRight.PrintInfo();
*/

  ArrayInterpreter array4;  // 2NX x NY
  array4.Allocate( LEN * 2 );
  val = 0;
  for( vtkm::Id i = 0; i < array4.GetNumberOfValues(); i++ )
    array4.GetPortalControl().Set(i, val++ );
  array4.InterpretAs2D( 2*NX, NY );
  typedef vtkm::cont::ArrayHandleConcatenate2DTopDown
          < ArrayInterpreter, Concat2DLeftRight >   Concat2DTopDownV2;
  Concat2DTopDownV2 topdown2( array4, arrayLeftRight );

  for( vtkm::Id j = 0; j < topdown2.GetDimY(); j++ )
  {
    for( vtkm::Id i = 0; i < topdown2.GetDimX(); i++ )
    {
      std::cout << topdown2.Get2D( i, j ) << "\t";
    }
    std::cout << std::endl;
  }

}

} 

int UnitTestArrayHandleInterpreter(int, char *[])
{
  using namespace UnitTestArrayHandleInterpreterNamespace;
  return vtkm::cont::testing::Testing::Run(TestArrayHandleInterpreter);
}
