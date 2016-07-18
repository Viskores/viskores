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
#ifndef vtk_m_ArrayHandleConcatenate_h
#define vtk_m_ArrayHandleConcatenate_h

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm {
namespace cont {
namespace internal {

template< typename PortalType1, typename PortalType2 >
class ArrayPortalConcatenate
{
public:
  typedef typename PortalType1::ValueType ValueType;

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalConcatenate() : portal1(), portal2() {}

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalConcatenate( const PortalType1 &p1, const PortalType2 &p2 )  
    : portal1( p1 ), portal2( p2 ) {}

  // Copy constructor
  template< typename OtherP1, typename OtherP2 >
  VTKM_EXEC_CONT_EXPORT
  ArrayPortalConcatenate( const ArrayPortalConcatenate< OtherP1, OtherP2 > &src )
    : portal1( src.GetPortal1() ), portal2( src.GetPortal2() ) {}

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    return this->portal1.GetNumberOfValues() +
           this->portal2.GetNumberOfValues() ;
  }
  
  VTKM_EXEC_CONT_EXPORT
  ValueType Get( vtkm::Id index) const
  {
    if( index < this->portal1.GetNumberOfValues() )
      return this->portal1.Get( index );
    else
      return this->portal2.Get( index - this->portal1.GetNumberOfValues() );
  }

  VTKM_EXEC_CONT_EXPORT
  void Set( vtkm::Id index, const ValueType &value ) const
  {
    if( index < this->portal1.GetNumberOfValues() )
      this->portal1.Set( index, value );
    else
      this->portal2.Set( index - this->portal1.GetNumberOfValues(), value );
  }

  VTKM_EXEC_CONT_EXPORT
  const PortalType1 &GetPortal1() const
  {
    return this->portal1;
  }

  VTKM_EXEC_CONT_EXPORT
  const PortalType2 &GetPortal2() const
  {
    return this->portal2;
  }

private:
  PortalType1 portal1;
  PortalType2 portal2;
};  // class ArrayPortalConcatenate

}   // namespace internal

template< typename ArrayHandleType1, typename ArrayHandleType2 >
class StorageTagConcatenate {};


namespace internal {

template< typename ArrayHandleType1, typename ArrayHandleType2 >
class Storage< typename ArrayHandleType1::ValueType,
               StorageTagConcatenate< ArrayHandleType1, ArrayHandleType2> >
{
public:
  typedef typename ArrayHandleType1::ValueType ValueType;
  typedef ArrayPortalConcatenate< typename ArrayHandleType1::PortalControl,
                                  typename ArrayHandleType2::PortalControl > PortalType;
  typedef ArrayPortalConcatenate<
          typename ArrayHandleType1::PortalConstControl,
          typename ArrayHandleType2::PortalConstControl > PortalConstType;

  VTKM_CONT_EXPORT
  Storage() : valid( false ) { }
  
  VTKM_CONT_EXPORT
  Storage( const ArrayHandleType1 &a1, const ArrayHandleType2 &a2 ) 
    : array1( a1 ), array2( a2 ), valid( true ) {};
  
  VTKM_CONT_EXPORT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT( this->valid );
    return PortalConstType( this->array1.GetPortalConstControl(),
                            this->array2.GetPortalConstControl() );
  }

  VTKM_CONT_EXPORT
  PortalType GetPortal()
  {
    VTKM_ASSERT( this->valid );
    return PortalType( this->array1.GetPortalControl(),
                       this->array2.GetPortalControl() );
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT( this->valid );
    return this->array1.GetNumberOfValues() + this->array2.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  void Allocate( vtkm::Id numberOfValues )
  {
    VTKM_ASSERT( this->valid );
    VTKM_ASSERT( numberOfValues > 0 );
    // throw an error; not a valid operation for this array handle.
  }

  VTKM_CONT_EXPORT
  void Shrink( vtkm::Id numberOfValues )
  {
    VTKM_ASSERT( this->valid );
    if( numberOfValues < this->array1.GetNumberOfValues() )
    {
      this->array1.Shrink( numberOfValues );
      this->array2.Shrink( 0 );
    }
    else
      this->array2.Shrink( numberOfValues - this->array1.GetNumberOfValues() );
  }

  VTKM_CONT_EXPORT
  void ReleaseResources( )
  {
    VTKM_ASSERT( this->valid );
    this->array1.ReleaseResources();
    this->array2.ReleaseResources();
  }

  VTKM_CONT_EXPORT
  const ArrayHandleType1 &GetArray1() const
  {
    VTKM_ASSERT( this->valid );
    return this->array1;
  }

  VTKM_CONT_EXPORT
  const ArrayHandleType2 &GetArray2() const 
  {
    VTKM_ASSERT( this->valid );
    return this->array2;
  }

private:
  ArrayHandleType1 array1;
  ArrayHandleType2 array2; 
  bool             valid;
};    // class Storage


template< typename ArrayHandleType1, typename ArrayHandleType2, typename Device >
class ArrayTransfer< typename ArrayHandleType1::ValueType,
                     StorageTagConcatenate< ArrayHandleType1, ArrayHandleType2>,
                     Device >
{
public:
  typedef typename ArrayHandleType1::ValueType ValueType;
  
private:
  typedef StorageTagConcatenate< ArrayHandleType1, ArrayHandleType2 > StorageTag;
  typedef vtkm::cont::internal::Storage< ValueType, StorageTag> StorageType;

public:
  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;

  typedef ArrayPortalConcatenate< 
        typename ArrayHandleType1::template ExecutionTypes< Device >::Portal,
        typename ArrayHandleType2::template ExecutionTypes< Device >::Portal > 
      PortalExecution;
  typedef ArrayPortalConcatenate< 
        typename ArrayHandleType1::template ExecutionTypes< Device >::PortalConst,
        typename ArrayHandleType2::template ExecutionTypes< Device >::PortalConst > 
      PortalConstExecution;

  VTKM_CONT_EXPORT
  ArrayTransfer( StorageType* storage ) 
      : array1( storage->GetArray1() ), array2( storage->GetArray2() ) {}

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    return this->array1.GetNumberOfValues() + this->array2.GetNumberOfValues() ;
  }

  VTKM_CONT_EXPORT
  PortalConstExecution PrepareForInput( bool vtkmNotUsed( updateData ) )
  {
    return PortalConstExecution( this->array1.PrepareForInput( Device() ),
                                 this->array2.PrepareForInput( Device() ));
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForInPlace( bool vtkmNotUsed( updateData ) )
  {
    return PortalExecution( this->array1.PrepareForInPlace( Device() ),
                            this->array2.PrepareForInPlace( Device() ));
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForOutput( vtkm::Id numberOfValues )
  {
    VTKM_ASSERT( numberOfValues > 0 );
    throw vtkm::cont::ErrorControlBadValue(
            "ArrayHandleConcatenate is derived and read-only.");
  }

  VTKM_CONT_EXPORT
  void RetrieveOutputData( StorageType* vtkmNotUsed(storage) ) const
  {
    // not need to implement
  }

  VTKM_CONT_EXPORT
  void Shrink( vtkm::Id numberOfValues )
  {
    if( numberOfValues < this->array1.GetNumberOfValues() )
    {
      this->array1.Shrink( numberOfValues );
      this->array2.Shrink( 0 );
    }
    else
      this->array2.Shrink( numberOfValues - this->array1.GetNumberOfValues() );
  }

  VTKM_CONT_EXPORT
  void ReleaseResources()
  {
    this->array1.ReleaseResourcesExecution();
    this->array2.ReleaseResourcesExecution();
  }  

private:
  ArrayHandleType1 array1;
  ArrayHandleType2 array2;

};

}
}
} // namespace vtkm::cont::internal



namespace vtkm {
namespace cont {

template< typename ArrayHandleType1, typename ArrayHandleType2 >
class ArrayHandleConcatenate 
        : public vtkm::cont::ArrayHandle< typename ArrayHandleType1::ValueType,
                 StorageTagConcatenate< ArrayHandleType1, ArrayHandleType2> >
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS( ArrayHandleConcatenate, 
      ( ArrayHandleConcatenate< ArrayHandleType1, ArrayHandleType2> ),
      ( vtkm::cont::ArrayHandle< typename ArrayHandleType1::ValueType, 
            StorageTagConcatenate< ArrayHandleType1, ArrayHandleType2 > > ));

private:
  typedef vtkm::cont::internal::Storage< ValueType, StorageTag > StorageType;

public:

  VTKM_CONT_EXPORT
  ArrayHandleConcatenate( const ArrayHandleType1 &array1, 
                          const ArrayHandleType2 &array2 )
      : Superclass( StorageType( array1, array2 ) )
  {}

};
      

template< typename ArrayHandleType1, typename ArrayHandleType2 >
VTKM_CONT_EXPORT
ArrayHandleConcatenate< ArrayHandleType1, ArrayHandleType2 >
make_ArrayHandleConcatenate( const ArrayHandleType1 &array1, 
                             const ArrayHandleType2 &array2 )
{
  return ArrayHandleConcatenate< ArrayHandleType1, ArrayHandleType2 >( array1, array2 ); 
}

}
}   // namespace vtkm::cont

#endif //vtk_m_ArrayHandleConcatenate_h
