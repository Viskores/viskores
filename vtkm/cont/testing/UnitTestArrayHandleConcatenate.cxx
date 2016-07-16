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

#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleConcatenate.h>

#include <vtkm/cont/testing/Testing.h>

namespace UnitTestArrayHandleConcatenateNamespace {

const vtkm::Id ARRAY_SIZE = 5;

void TestArrayHandleConcatenate()
{
  vtkm::cont::ArrayHandleIndex array1(   ARRAY_SIZE );
  vtkm::cont::ArrayHandleIndex array2( 2*ARRAY_SIZE );

  vtkm::cont::ArrayHandleConcatenate< vtkm::cont::ArrayHandleIndex,
                                      vtkm::cont::ArrayHandleIndex >
       array3( array1, array2 );

  vtkm::cont::ArrayHandleIndex array4( ARRAY_SIZE );
  vtkm::cont::ArrayHandleConcatenate< 
      vtkm::cont::ArrayHandleConcatenate< vtkm::cont::ArrayHandleIndex,   // 1st 
                                          vtkm::cont::ArrayHandleIndex >, // ArrayHandle
      vtkm::cont::ArrayHandleIndex >                                  // 2nd ArrayHandle
          array5;
  {
    array5  = vtkm::cont::make_ArrayHandleConcatenate( array3, array4 );
  }

  for (vtkm::Id index = 0; index < array5.GetNumberOfValues(); index++)
  {
    std::cout << array5.GetPortalConstControl().Get( index ) << std::endl; 
  }
}

} // namespace UnitTestArrayHandleIndexNamespace

int UnitTestArrayHandleConcatenate(int, char *[])
{
  using namespace UnitTestArrayHandleConcatenateNamespace;
  return vtkm::cont::testing::Testing::Run(TestArrayHandleConcatenate);
}
