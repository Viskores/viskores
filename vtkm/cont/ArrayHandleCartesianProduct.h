//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleCartesianProduct_h
#define vtk_m_cont_ArrayHandleCartesianProduct_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Assert.h>
#include <vtkm/cont/ErrorControlBadAllocation.h>

namespace vtkm {
namespace exec {
namespace internal {

/// \brief An array portal that acts as a 3D cartesian product of 3 arrays.
/// for the execution environment

template<typename ValueType_,
         typename PortalTypeFirst_,
         typename PortalTypeSecond_,
         typename PortalTypeThird_>
class ArrayPortalExecCartesianProduct
{
public:
  typedef ValueType_ ValueType;
  typedef ValueType_ IteratorType;
  typedef PortalTypeFirst_ PortalTypeFirst;
  typedef PortalTypeSecond_ PortalTypeSecond;
  typedef PortalTypeThird_ PortalTypeThird;

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalExecCartesianProduct()
  : PortalFirst(), PortalSecond(), PortalThird()
  {  } //needs to be host and device so that cuda can create lvalue of these

  VTKM_CONT_EXPORT
  ArrayPortalExecCartesianProduct(const PortalTypeFirst  &portalfirst,
	                          const PortalTypeSecond &portalsecond,
	                          const PortalTypeThird &portalthird)
      : PortalFirst(portalfirst), PortalSecond(portalsecond), PortalThird(portalthird)
  {  }

  /// Copy constructor for any other ArrayPortalExecCartesianProduct with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///

  template<class OtherV, class OtherP1, class OtherP2, class OtherP3>
  VTKM_CONT_EXPORT
  ArrayPortalExecCartesianProduct(const ArrayPortalExecCartesianProduct<OtherV,OtherP1,OtherP2,OtherP3> &src)
    : PortalFirst(src.GetPortalFirst()),
      PortalSecond(src.GetPortalSecond()),
      PortalThird(src.GetPortalThird())
  {  }


  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
      return this->PortalFirst.GetNumberOfValues() *
	  this->PortalSecond.GetNumberOfValue() *
	  this->PortalThird.GetNumberOfValue();
  }

  VTKM_EXEC_EXPORT
  ValueType Get(vtkm::Id index) const
  {
      vtkm::Id dim1 = this->PortalFirst.GetNumberOfValues();
      vtkm::Id dim2 = this->PortalSecond.GetNumberOfValues();
      vtkm::Id dim12 = dim1*dim2;
      vtkm::Id idx12 = index % dim12;
      vtkm::Id i1 = idx12 % dim1;
      vtkm::Id i2 = idx12 / dim1;
      vtkm::Id i3 = index / dim12;
      
      return vtkm::make_Vec(this->PortalFirst.Get(i1),
			    this->PortalSecond.Get(i2),
			    this->PortalThird.Get(i3));
  }

  VTKM_EXEC_EXPORT
  void Set(vtkm::Id index, const ValueType &value) const
  {
      vtkm::Id dim1 = this->PortalFirst.GetNumberOfValues();
      vtkm::Id dim2 = this->PortalSecond.GetNumberOfValues();
      vtkm::Id dim12 = dim1*dim2;
      vtkm::Id idx12 = index % dim12;

      vtkm::Id i1 = idx12 % dim1;
      vtkm::Id i2 = idx12 / dim1;
      vtkm::Id i3 = index / dim12;

      this->PortalFirst.Set(i1, value[0]);
      this->PortalSecond.Set(i2, value[1]);
      this->PortalThird.Set(i3, value[2]);
  }

  VTKM_EXEC_CONT_EXPORT
  const PortalTypeFirst &GetFirstPortal() const { return this->PortalFirst; }

  VTKM_EXEC_CONT_EXPORT
  const PortalTypeSecond &GetSecondPortal() const { return this->PortalSecond; }

  VTKM_EXEC_CONT_EXPORT
  const PortalTypeThird &GetThirdPortal() const { return this->PortalThird; }


private:
  PortalTypeFirst PortalFirst;
  PortalTypeSecond PortalSecond;
  PortalTypeThird PortalThird;
};

}
}
} // namespace vtkm::exec::internal


namespace vtkm {
namespace cont {

namespace internal {

/// \brief An array portal that zips two portals together into a single value
/// for the control environment
template<typename ValueType_,
         typename PortalTypeFirst,
         typename PortalTypeSecond,
         typename PortalTypeThird>
class ArrayPortalContCartesianProduct
{
public:
  typedef ValueType_ ValueType;
  typedef ValueType_ IteratorType;

  VTKM_CONT_EXPORT
  ArrayPortalContCartesianProduct(const PortalTypeFirst  &portalfirst = PortalTypeFirst(),
				  const PortalTypeSecond &portalsecond = PortalTypeSecond(),
				  const PortalTypeSecond &portalthird = PortalTypeThird())
      : PortalFirst(portalfirst), PortalSecond(portalsecond), PortalThird(portalthird)
  {  }

  /// Copy constructor for any other ArrayPortalContCartesianProduct with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<class OtherV, class OtherP1, class OtherP2, class OtherP3>
  VTKM_CONT_EXPORT
  ArrayPortalContCartesianProduct(const ArrayPortalContCartesianProduct<OtherV,
				  OtherP1,OtherP2,OtherP3> &src)
    : PortalFirst(src.GetPortalFirst()),
      PortalSecond(src.GetPortalSecond()),
      PortalThird(src.GetPortalThird())
  {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
      return this->PortalFirst.GetNumberOfValues() *
	  this->PortalSecond.GetNumberOfValues() *
	  this->PortalThird.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  ValueType Get(vtkm::Id index) const
  {
      vtkm::Id dim1 = this->PortalFirst.GetNumberOfValues();
      vtkm::Id dim2 = this->PortalSecond.GetNumberOfValues();
      vtkm::Id dim12 = dim1*dim2;
      vtkm::Id idx12 = index % dim12;
      vtkm::Id i1 = idx12 % dim1;
      vtkm::Id i2 = idx12 / dim1;
      vtkm::Id i3 = index / dim12;
      return vtkm::make_Vec(this->PortalFirst.Get(i1),
			    this->PortalSecond.Get(i2),
			    this->PortalThird.Get(i3));
  }

  VTKM_CONT_EXPORT
  void Set(vtkm::Id index, const ValueType &value) const
  {
      vtkm::Id dim1 = this->PortalFirst.GetNumberOfValues();
      vtkm::Id dim2 = this->PortalSecond.GetNumberOfValues();
      vtkm::Id dim12 = dim1*dim2;
      vtkm::Id idx12 = index % dim12;

      vtkm::Id i1 = idx12 % dim1;
      vtkm::Id i2 = idx12 / dim1;
      vtkm::Id i3 = index / dim12;

      this->PortalFirst.Set(i1, value[0]);
      this->PortalSecond.Set(i2, value[1]);
      this->PortalThird.Set(i3, value[2]);
  }

  VTKM_CONT_EXPORT
  const PortalTypeFirst &GetFirstPortal() const { return this->PortalFirst; }

  VTKM_CONT_EXPORT
  const PortalTypeSecond &GetSecondPortal() const { return this->PortalSecond; }

  VTKM_CONT_EXPORT
  const PortalTypeSecond &GetThirdPortal() const { return this->PortalThird; }


private:
  PortalTypeFirst PortalFirst;
  PortalTypeSecond PortalSecond;
  PortalTypeThird PortalThird;
};

template<typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
struct StorageTagCartesianProduct {  };

/// This helper struct defines the value type for a zip container containing
/// the given two array handles.
///
template<typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
struct ArrayHandleCartesianProductTraits {
  /// The ValueType (a pair containing the value types of the two arrays).
  ///
  typedef vtkm::Vec<typename FirstHandleType::ValueType,3> ValueType;

  /// The appropriately templated tag.
  ///
  typedef StorageTagCartesianProduct<FirstHandleType,SecondHandleType,ThirdHandleType> Tag;

  /// The superclass for ArrayHandleCartesianProduct.
  ///
  typedef vtkm::cont::ArrayHandle<ValueType,Tag> Superclass;
};


template<typename T, typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
class Storage<T, StorageTagCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType > >
{
  VTKM_IS_ARRAY_HANDLE(FirstHandleType);
  VTKM_IS_ARRAY_HANDLE(SecondHandleType);
  VTKM_IS_ARRAY_HANDLE(ThirdHandleType);

public:
  typedef T ValueType;

  typedef ArrayPortalContCartesianProduct< ValueType,
                          typename FirstHandleType::PortalControl,
			  typename SecondHandleType::PortalControl,
			  typename ThirdHandleType::PortalControl> PortalType;
  typedef ArrayPortalContCartesianProduct< ValueType,
                          typename FirstHandleType::PortalConstControl,
			  typename SecondHandleType::PortalConstControl,
			  typename ThirdHandleType::PortalConstControl>
                                                               PortalConstType;

  VTKM_CONT_EXPORT
  Storage() : FirstArray(), SecondArray(), ThirdArray() {  }

  VTKM_CONT_EXPORT
  Storage(const FirstHandleType &array1, const SecondHandleType &array2, const ThirdHandleType &array3)
      : FirstArray(array1), SecondArray(array2), ThirdArray(array3)
  {

  }

  VTKM_CONT_EXPORT
  PortalType GetPortal()
  {
      return PortalType(this->FirstArray.GetPortalControl(),
			this->SecondArray.GetPortalControl(),
			this->ThirdArray.GetPortalControl());
  }

  VTKM_CONT_EXPORT
  PortalConstType GetPortalConst() const
  {
      return PortalConstType(this->FirstArray.GetPortalConstControl(),
			     this->SecondArray.GetPortalConstControl(),
			     this->ThirdArray.GetPortalConstControl());
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
      return this->FirstArray.GetNumberOfValues() *
	  this->SecondArray.GetNumberOfValues() *
	  this->ThirdArray.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  void Allocate(vtkm::Id /*numberOfValues*/)
  {
      throw vtkm::cont::ErrorControlBadAllocation("Does not make sense.");
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id /*numberOfValues*/)
  {
      throw vtkm::cont::ErrorControlBadAllocation("Does not make sense.");
  }

  VTKM_CONT_EXPORT
  void ReleaseResources()
  {
    // This request is ignored since it is asking to release the resources
    // of the arrays, which may be used elsewhere.
  }

  VTKM_CONT_EXPORT
  const FirstHandleType &GetFirstArray() const
  {
      return this->FirstArray;
  }

  VTKM_CONT_EXPORT
  const SecondHandleType &GetSecondArray() const
  {
      return this->SecondArray;
  }

  VTKM_CONT_EXPORT
  const ThirdHandleType &GetThirdArray() const
  {
      return this->ThirdArray;
  }

private:
  FirstHandleType FirstArray;
  SecondHandleType SecondArray;
  ThirdHandleType ThirdArray;
};

template<typename T,
         typename FirstHandleType,
         typename SecondHandleType,
         typename ThirdHandleType,
         typename Device>
class ArrayTransfer<
    T, StorageTagCartesianProduct<FirstHandleType,SecondHandleType,ThirdHandleType>, Device>
{
  typedef StorageTagCartesianProduct<FirstHandleType,SecondHandleType,ThirdHandleType> StorageTag;
  typedef vtkm::cont::internal::Storage<T, StorageTag> StorageType;

public:
  typedef T ValueType;

  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;

  typedef vtkm::exec::internal::ArrayPortalExecCartesianProduct<
      ValueType,
      typename FirstHandleType::template ExecutionTypes<Device>::Portal,
      typename SecondHandleType::template ExecutionTypes<Device>::Portal,
      typename ThirdHandleType::template ExecutionTypes<Device>::Portal
      > PortalExecution;

  typedef vtkm::exec::internal::ArrayPortalExecCartesianProduct<
      ValueType,
      typename FirstHandleType::template ExecutionTypes<Device>::PortalConst,
      typename SecondHandleType::template ExecutionTypes<Device>::PortalConst,
      typename ThirdHandleType::template ExecutionTypes<Device>::PortalConst
      > PortalConstExecution;

  VTKM_CONT_EXPORT
  ArrayTransfer(StorageType *storage)
    :  FirstArray(storage->GetFirstArray()),
       SecondArray(storage->GetSecondArray()),
       ThirdArray(storage->GetThirdArray())
    {  }


  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    return this->FirstArray.GetNumberOfValues() *
	this->SecondArray.GetNumberOfValues() *
	this->ThirdArray.GetNumberOfValues();
	
  }

  VTKM_CONT_EXPORT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData)) {
    return PortalConstExecution(this->FirstArray.PrepareForInput(Device()),
                                this->SecondArray.PrepareForInput(Device()));
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
      throw vtkm::cont::ErrorControlBadAllocation("Does not make sense.");
      return PortalExecution(this->FirstArray.PrepareForInput(Device()),
			     this->SecondArray.PrepareForInPlace(Device()),
			     this->ThirdArray.PrepareForInPlace(Device()));
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
      throw vtkm::cont::ErrorControlBadAllocation("Does not make sense.");
      return PortalExecution(this->FirstArray.PrepareForOutput(numberOfValues, Device()),
			     this->SecondArray.PrepareForOutput(numberOfValues, Device()),
			     this->ThirdArray.PrepareForOutput(numberOfValues, Device()));
  }

  VTKM_CONT_EXPORT
  void RetrieveOutputData(StorageType *vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // first and second array handles should automatically retrieve the
    // output data as necessary.
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id /*numberOfValues*/)
  {
      throw vtkm::cont::ErrorControlBadAllocation("Does not make sense.");
  }

  VTKM_CONT_EXPORT
  void ReleaseResources()
  {
    this->FirstArray.ReleaseResourcesExecution();
    this->SecondArray.ReleaseResourcesExecution();
    this->ThirdArray.ReleaseResourcesExecution();
  }


private:
  FirstHandleType FirstArray;
  SecondHandleType SecondArray;
  ThirdHandleType ThirdArray;
};
} // namespace internal

/// ArrayHandleCartesianProduct is a specialization of ArrayHandle. It takes two delegate
/// array handle and makes a new handle that access the corresponding entries
/// in these arrays as a pair.
///
template<typename FirstHandleType,
         typename SecondHandleType,
	 typename ThirdHandleType>
class ArrayHandleCartesianProduct
    : public internal::ArrayHandleCartesianProductTraits<FirstHandleType,SecondHandleType,ThirdHandleType>::Superclass
{
  // If the following line gives a compile error, then the FirstHandleType
  // template argument is not a valid ArrayHandle type.
  VTKM_IS_ARRAY_HANDLE(FirstHandleType);
  VTKM_IS_ARRAY_HANDLE(SecondHandleType);
  VTKM_IS_ARRAY_HANDLE(ThirdHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
      ArrayHandleCartesianProduct,
      (ArrayHandleCartesianProduct<FirstHandleType,SecondHandleType,ThirdHandleType>),
      (typename internal::ArrayHandleCartesianProductTraits<
       FirstHandleType,SecondHandleType,ThirdHandleType>::Superclass));

private:
  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

public:
  VTKM_CONT_EXPORT
  ArrayHandleCartesianProduct(const FirstHandleType &firstArray,
			      const SecondHandleType &secondArray,
			      const ThirdHandleType &thirdArray)
      : Superclass(StorageType(firstArray, secondArray, thirdArray)) { }
};

/// A convenience function for creating an ArrayHandleCartesianProduct. It takes the two
/// arrays to be zipped together.
///
template<typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
VTKM_CONT_EXPORT
vtkm::cont::ArrayHandleCartesianProduct<FirstHandleType,SecondHandleType,ThirdHandleType>
make_ArrayHandleCartesianProduct(const FirstHandleType &first,
				 const SecondHandleType &second,
				 const ThirdHandleType &third)
{
    return ArrayHandleCartesianProduct<FirstHandleType,
				       SecondHandleType,
				       ThirdHandleType>(first, second,third);
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleCartesianProduct_h
