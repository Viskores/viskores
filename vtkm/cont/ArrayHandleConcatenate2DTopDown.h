//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
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
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_ArrayHandleConcatenate2DTopDown_h
#define vtk_m_ArrayHandleConcatenate2DTopDown_h

#include <vtkm/cont/ArrayHandleConcatenate.h>

namespace vtkm {
namespace cont {


template< typename ArrayHandleType1, typename ArrayHandleType2 >
class ArrayHandleConcatenate2DTopDown : public vtkm::cont::ArrayHandleConcatenate
                                        <ArrayHandleType1, ArrayHandleType2 >
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS( ArrayHandleConcatenate2DTopDown, 
      ( ArrayHandleConcatenate2DTopDown   < ArrayHandleType1, ArrayHandleType2> ),
      ( vtkm::cont::ArrayHandleConcatenate< ArrayHandleType1, ArrayHandleType2> ));

private:
  vtkm::Id    dimX, dimY;
  vtkm::Id    dimYTop, dimYDown;
  ArrayHandleType1  array1;
  ArrayHandleType2  array2;

public:
  // constructor
  VTKM_CONT_EXPORT
  ArrayHandleConcatenate2DTopDown( const ArrayHandleType1 &arr1, 
                                   const ArrayHandleType2 &arr2 )
                     : Superclass( arr1, arr2 )
  {
    if( arr1.IsValid2D() && arr2.IsValid2D() )
    {
      if( arr1.GetDimX() == arr2.GetDimX() )
      {
        this->dimX      = arr1.GetDimX();
        this->dimYTop   = arr1.GetDimY();
        this->dimYDown  = arr2.GetDimY();
        this->dimY      = dimYTop + dimYDown;

        this->array1    = arr1;
        this->array2    = arr2;
        array1.CopyInterpreterInfo( arr1 );
        array2.CopyInterpreterInfo( arr2 );
      }
      else
        throw vtkm::cont::ErrorControlBadValue(
              "ArrayHandleConcatenate2DTopDown::Not the same X dimension to concatenate.");
    }
    else
      throw vtkm::cont::ErrorControlInternal(
            "ArrayHandleConcatenate2DTopDown::Not valid 2D matrices to concatenate.");
  }

  vtkm::Id GetDimX() const  {return this->dimX;}
  vtkm::Id GetDimY() const  {return this->dimY;}

  ValueType Get2D( vtkm::Id x, vtkm::Id y ) const
  {
    if( y < dimYTop )
      return array1.Get2D( x, y );
    else
      return array2.Get2D( x, y - dimYTop );
  }

  void Set2D( vtkm::Id x, vtkm::Id y, ValueType val )
  {
    if( y < dimYTop )
      array1.Set2D( x, y, val );
    else
      array2.Set2D( x, y - dimYTop, val );
  }

};
      

template< typename ArrayHandleType1, typename ArrayHandleType2 >
VTKM_CONT_EXPORT
ArrayHandleConcatenate2DTopDown< ArrayHandleType1, ArrayHandleType2 >
make_ArrayHandleConcatenate2DTopDown( const ArrayHandleType1 &array1, 
                                      const ArrayHandleType2 &array2 )
{
  return ArrayHandleConcatenate2DTopDown< ArrayHandleType1, ArrayHandleType2 >( array1, array2 ); 
}

}
}   // namespace vtkm::cont

#endif 
