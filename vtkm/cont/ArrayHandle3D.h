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
#ifndef vtk_m_ArrayHandle2D_h
#define vtk_m_ArrayHandle2D_h

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm {
namespace cont {
namespace internal {

template< typename PortalType >
class ArrayPortal3D
{
public:
  typedef typename PortalType1::ValueType ValueType;

  // constructor
  VTKM_EXEC_CONT_EXPORT
  ArrayPortal3D() : portal() {}

  // constructor
  VTKM_EXEC_CONT_EXPORT
  ArrayPortal3D( const PortalType &p )  : portal( p )  {}

  // Copy constructor
  template< typename OtherP >
  VTKM_EXEC_CONT_EXPORT
  ArrayPortal3D( const ArrayPortal3D<OtherP> &src ) : portal(src.GetPortal()) {}

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    return this->portal.GetNumberOfValues();
  }
  
  VTKM_EXEC_CONT_EXPORT
  ValueType Get( vtkm::Id index) const
  {
    return this->portal.Get( index );
  }

  VTKM_EXEC_CONT_EXPORT
  void Set( vtkm::Id index, const ValueType &value ) const
  {
    this->portal.Set( index, value );
  }

  VTKM_EXEC_CONT_EXPORT
  const PortalType &GetPortal() const
  {
    return this->portal;
  }

private:
  PortalType portal;
}; 

}   // namespace internal


template< typename ArrayHandleType1 >
class StorageTag2D {};


namespace internal {

template< typename ArrayHandleType >
class Storage< typename ArrayHandleType::ValueType, StorageTag2D<ArrayHandleType> >
{
public:
  typedef typename ArrayHandleType::ValueType ValueType;
  typedef ArrayPortal2D< typename ArrayHandleType::PortalControl >      PortalType;
  typedef ArrayPortal2D< typename ArrayHandleType::PortalConstControl > PortalConstType;

  VTKM_CONT_EXPORT
  Storage() : valid( false ) { }
  
  VTKM_CONT_EXPORT
  Storage( const ArrayHandleType &a ) : array( a ), valid( true ) {};
  
  VTKM_CONT_EXPORT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT( this->valid );
    return PortalConstType( this->array1.GetPortalConstControl() );
  }

  VTKM_CONT_EXPORT
  PortalType GetPortal()
  {
    VTKM_ASSERT( this->valid );
    return PortalType( this->array.GetPortalControl() );
  }

/* 
 * working toward here
 */
  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT( this->valid );
    return this->array1.GetNumberOfValues() + this->array2.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  void Allocate( vtkm::Id numberOfValues )
  {
    (void)numberOfValues;   // dummy statement to avoid a warning
    throw vtkm::cont::ErrorControlInternal(
          "ArrayHandleConcatenate should not be allocated explicitly. " );
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
    (void)numberOfValues;   // dummy statement to avoid a warning
    throw vtkm::cont::ErrorControlInternal(
          "ArrayHandleConcatenate is derived and read-only. " );
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

#endif //vtk_m_ArrayHandle2D_h
