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
#ifndef vtk_m_ArrayHandleConcatenate2DLeftRight_h
#define vtk_m_ArrayHandleConcatenate2DLeftRight_h

#include <vtkm/cont/ArrayHandleConcatenate.h>

namespace vtkm {
namespace cont {


template< typename ArrayHandleType1, typename ArrayHandleType2 >
class ArrayHandleConcatenate2DLeftRight : public vtkm::cont::ArrayHandleConcatenate
                                        <ArrayHandleType1, ArrayHandleType2 >
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS( ArrayHandleConcatenate2DLeftRight, 
      ( ArrayHandleConcatenate2DLeftRight   < ArrayHandleType1, ArrayHandleType2> ),
      ( vtkm::cont::ArrayHandleConcatenate< ArrayHandleType1, ArrayHandleType2> ));

private:
  ArrayHandleType1  array1;
  ArrayHandleType2  array2;

  template< typename ArrayType >
  bool Validate2DArray ( const ArrayType& arr )
  {
    return ( arr.GetNumberOfValues() == arr.GetDimX() * arr.GetDimY() );
  }

public:
  // constructor
  VTKM_CONT_EXPORT
  ArrayHandleConcatenate2DLeftRight( const ArrayHandleType1 &arr1, 
                                     const ArrayHandleType2 &arr2 )
                     : Superclass( arr1, arr2 )
  {
    if( this->Validate2DArray(arr1) && this->Validate2DArray(arr2) )
    {
      if( arr1.GetDimY() == arr2.GetDimY() )
      {
        this->array1    = arr1;
        this->array2    = arr2;
        this->array1.CopyDimInfo( arr1 );
        this->array2.CopyDimInfo( arr2 );
      }
      else
        throw vtkm::cont::ErrorControlBadValue(
              "ArrayHandleConcatenate2DLeftRight::Not the same Y dimension to concatenate.");
    }
    else
      throw vtkm::cont::ErrorControlInternal(
            "ArrayHandleConcatenate2DLeftRight::Not valid 2D matrices to concatenate.");
  }

  vtkm::Id GetDimX() const
  {
    return ( array1.GetDimX() + array2.GetDimX() );
  }
  vtkm::Id GetDimY() const
  {
    return array1.GetDimY();
  }

  const ArrayHandleType1& GetArray1() const  { return array1; }
  const ArrayHandleType2& GetArray2() const  { return array2; }

  void CopyDimInfo( const ArrayHandleConcatenate2DLeftRight& src )
  {
    this->array1  = src.GetArray1();
    this->array2  = src.GetArray2();
    this->array1.CopyDimInfo( src.GetArray1() );
    this->array2.CopyDimInfo( src.GetArray2() );
  }

  void PrintInfo()
  {
    std::cout << "Concatenate2DLeftRight Dimensions: (" 
              << GetDimX() << ", " << GetDimY() << ")" << std::endl;
    std::cout << "  its two member arrays: " << std::endl;
    std::cout << "  ";  array1.PrintInfo();
    std::cout << "  ";  array2.PrintInfo();
  }

  ValueType Get2D( vtkm::Id x, vtkm::Id y ) const
  {
    if( x < array1.GetDimX() )
      return array1.Get2D( x, y );
    else
      return array2.Get2D( x - array1.GetDimX(), y );
  }

  void Set2D( vtkm::Id x, vtkm::Id y, ValueType val )
  {
    if( x < array1.GetDimX() )
      array1.Set2D( x, y, val );
    else
      array2.Set2D( x - array1.GetDimX(), y, val );
  }

};
      

template< typename ArrayHandleType1, typename ArrayHandleType2 >
VTKM_CONT_EXPORT
ArrayHandleConcatenate2DLeftRight< ArrayHandleType1, ArrayHandleType2 >
make_ArrayHandleConcatenate2DLeftRight( const ArrayHandleType1 &array1, 
                                        const ArrayHandleType2 &array2 )
{
  return ArrayHandleConcatenate2DLeftRight< ArrayHandleType1, ArrayHandleType2 >( array1, array2 ); 
}

}
}   // namespace vtkm::cont

#endif 
