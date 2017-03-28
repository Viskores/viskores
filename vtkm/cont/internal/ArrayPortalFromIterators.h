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
#ifndef vtk_m_cont_internal_ArrayPortalFromIterators_h
#define vtk_m_cont_internal_ArrayPortalFromIterators_h

#include <vtkm/Assert.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ErrorBadAllocation.h>

#include <iterator>
#include <limits>

#include <type_traits>

namespace vtkm {
namespace cont {
namespace internal {

template<typename IteratorT, typename Enable= void>
class ArrayPortalFromIterators;

/// This templated implementation of an ArrayPortal allows you to adapt a pair
/// of begin/end iterators to an ArrayPortal interface.
///
template<class IteratorT>
class ArrayPortalFromIterators<IteratorT,
                               typename std::enable_if<
                                !std::is_const< typename std::remove_pointer<IteratorT>::type >::value >::type >
{
public:
  typedef typename std::iterator_traits<IteratorT>::value_type ValueType;
  typedef IteratorT IteratorType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_CONT
  ArrayPortalFromIterators() {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_CONT
  ArrayPortalFromIterators(IteratorT begin, IteratorT end)
    : BeginIterator(begin)
  {
    typename std::iterator_traits<IteratorT>::difference_type numberOfValues =
      std::distance(begin, end);
    VTKM_ASSERT(numberOfValues >= 0);
#ifndef VTKM_USE_64BIT_IDS
    if (numberOfValues > (std::numeric_limits<vtkm::Id>::max)())
    {
      throw vtkm::cont::ErrorBadAllocation(
        "Distance of iterators larger than maximum array size. "
        "To support larger arrays, try turning on VTKM_USE_64BIT_IDS.");
    }
#endif // !VTKM_USE_64BIT_IDS
    this->NumberOfValues = static_cast<vtkm::Id>(numberOfValues);
  }

  /// Copy constructor for any other ArrayPortalFromIterators with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<class OtherIteratorT>
  VTKM_CONT
  ArrayPortalFromIterators(const ArrayPortalFromIterators<OtherIteratorT> &src)
    : BeginIterator(src.GetIteratorBegin()), NumberOfValues(src.GetNumberOfValues())
  {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->NumberOfValues;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    return *this->IteratorAt(index);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const
  {
    *(this->BeginIterator + index) = value;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorT GetIteratorBegin() const {
    return this->BeginIterator;
  }

private:
  IteratorT BeginIterator;
  vtkm::Id NumberOfValues;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorT IteratorAt(vtkm::Id index) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->GetNumberOfValues());

    return this->BeginIterator + index;
  }
};

template<class IteratorT>
class ArrayPortalFromIterators<IteratorT,
                               typename std::enable_if<
                                std::is_const< typename std::remove_pointer<IteratorT>::type >::value >::type >
{
public:
  typedef typename std::iterator_traits<IteratorT>::value_type ValueType;
  typedef IteratorT IteratorType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_CONT
  ArrayPortalFromIterators() {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_CONT
  ArrayPortalFromIterators(IteratorT begin, IteratorT end)
    : BeginIterator(begin)
  {
    typename std::iterator_traits<IteratorT>::difference_type numberOfValues =
      std::distance(begin, end);
    VTKM_ASSERT(numberOfValues >= 0);
#ifndef VTKM_USE_64BIT_IDS
    if (numberOfValues > (std::numeric_limits<vtkm::Id>::max)())
    {
      throw vtkm::cont::ErrorBadAllocation(
        "Distance of iterators larger than maximum array size. "
        "To support larger arrays, try turning on VTKM_USE_64BIT_IDS.");
    }
#endif // !VTKM_USE_64BIT_IDS
    this->NumberOfValues = static_cast<vtkm::Id>(numberOfValues);
  }

  /// Copy constructor for any other ArrayPortalFromIterators with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<class OtherIteratorT>
  VTKM_CONT
  ArrayPortalFromIterators(const ArrayPortalFromIterators<OtherIteratorT> &src)
    : BeginIterator(src.GetIteratorBegin()), NumberOfValues(src.GetNumberOfValues())
  {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->NumberOfValues;
  }

  //DRP
  VTKM_SUPPRESS_EXEC_WARNINGS
  template<typename T1, typename T2>
  VTKM_EXEC_CONT
  inline void CopyRangeInto(const T1 &indices, T2 &vals) const
  {
      std::cout<<" WRONG CopyRangeInto()"<<std::endl;
  }

  //DRP
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  inline void CopyRangeInto(const vtkm::Vec<vtkm::Id,8> &indices, vtkm::Vec<ValueType,8> &vals) const
  {
      //std::cout<<"Inside RIGHT CopyRangeInto()"<<std::endl;
      vals[0] = *(this->BeginIterator + indices[0]);
      vals[1] = *(this->BeginIterator + indices[1]);
      vals[2] = *(this->BeginIterator + indices[2]);
      vals[3] = *(this->BeginIterator + indices[3]);
      vals[4] = *(this->BeginIterator + indices[4]);
      vals[5] = *(this->BeginIterator + indices[5]);
      vals[6] = *(this->BeginIterator + indices[6]);
      vals[7] = *(this->BeginIterator + indices[7]);
  }
    
#if 0
  VTKM_SUPPRESS_EXEC_WARNINGS
  //  template<vtkm::IdComponent N, vtkm::IdComponent M>  
  VTKM_EXEC_CONT
  //  inline void CopyRangeInto(const vtkm::Vec<vtkm::Id, N> &indices, vtkm::Vec<ValueType, M> &vals) const
  inline void CopyRangeInto(const vtkm::Vec<vtkm::Id,8> &indices, vtkm::Vec<ValueType,8> &vals) const
  {
      std::cout<<"Inside CopyRangeInto()"<<std::endl;
      /*
      for (int i = 0; i < N; i++)
          vals[i] = *(this->BeginIterator + indices[i]);
      */
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  inline void CopyRangeInto(const vtkm::Vec<vtkm::Id,4> &indices, vtkm::Vec<ValueType,8> &vals) const
  {
      std::cout<<"Inside CopyRangeInto()"<<std::endl;
      /*
      for (int i = 0; i < N; i++)
          vals[i] = *(this->BeginIterator + indices[i]);
      */
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  inline void CopyRangeInto(const vtkm::Vec<vtkm::Id,4> &indices, vtkm::Vec<double,8> &vals) const
  {
      std::cout<<"Inside CopyRangeInto()"<<std::endl;
      /*
      for (int i = 0; i < N; i++)
          vals[i] = *(this->BeginIterator + indices[i]);
      */
  }
#endif

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  //DRP
  /*  inline*/ ValueType Get(vtkm::Id index) const
  {
    return *this->IteratorAt(index);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  inline ValueType Get_1(const vtkm::Vec<vtkm::Id,8> &indices) const
  {
      std::cout<<"In Get_1()"<<std::endl;
      //return this->BeginIterator + 0; //indices[0];
      return *this->IteratorAt(indices[0]);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  inline void Get8(const vtkm::Vec<vtkm::Id,8> &indices, vtkm::Vec<ValueType,8> &vals) const
  {
  }

  //DRP
#if 0
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <>
  VTKM_EXEC_CONT
  inline void Get8(const vtkm::Vec<vtkm::Id,8> &indices, vtkm::Vec<ValueType,8> &vals) const
  {
      //std::cout<<"HI THERE"<<std::endl;
      vals[0] = *(this->BeginIterator + indices[0]);
      vals[1] = *(this->BeginIterator + indices[1]);
      vals[2] = *(this->BeginIterator + indices[2]);
      vals[3] = *(this->BeginIterator + indices[3]);
      vals[4] = *(this->BeginIterator + indices[4]);
      vals[5] = *(this->BeginIterator + indices[5]);
      vals[6] = *(this->BeginIterator + indices[6]);
      vals[7] = *(this->BeginIterator + indices[7]);
      
      /*
      vals[0] = this->BeginIterator + index;
      vals[1] = this->BeginIterator + index + 1;
      vals[2] = this->BeginIterator + index + 2;
      vals[3] = this->BeginIterator + index + 3;
      vals[4] = this->BeginIterator + index + 4;
      vals[5] = this->BeginIterator + index + 5;
      vals[6] = this->BeginIterator + index + 6;
      vals[7] = this->BeginIterator + index + 7;
      */
  }
#endif

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void Set(vtkm::Id vtkmNotUsed(index), const ValueType& vtkmNotUsed(value)) const
  {
#if ! (defined(VTKM_MSVC) && defined(VTKM_CUDA))
    VTKM_ASSERT(false && "Attempted to write to constant array.");
#endif
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorT GetIteratorBegin() const {
    return this->BeginIterator;
  }

private:
  IteratorT BeginIterator;
  vtkm::Id NumberOfValues;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  //DRP
  /*  inline*/ IteratorT IteratorAt(vtkm::Id index) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->GetNumberOfValues());

    return this->BeginIterator + index;
  }
};

}
}
} // namespace vtkm::cont::internal

namespace vtkm {
namespace cont {

/// Partial specialization of \c ArrayPortalToIterators for \c
/// ArrayPortalFromIterators. Returns the original array rather than
/// the portal wrapped in an \c IteratorFromArrayPortal.
///
template<typename _IteratorType>
class ArrayPortalToIterators<
    vtkm::cont::internal::ArrayPortalFromIterators<_IteratorType> >
{
  typedef vtkm::cont::internal::ArrayPortalFromIterators<_IteratorType>
      PortalType;
public:
#if !defined(VTKM_MSVC) || (defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL == 0)
  typedef _IteratorType IteratorType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalToIterators(const PortalType &portal)
    : Iterator(portal.GetIteratorBegin()),
      NumberOfValues(portal.GetNumberOfValues())
  {  }

#else // VTKM_MSVC
  // The MSVC compiler issues warnings when using raw pointer math when in
  // debug mode. To keep the compiler happy (and add some safety checks),
  // wrap the iterator in checked_array_iterator.
  typedef stdext::checked_array_iterator<_IteratorType> IteratorType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalToIterators(const PortalType &portal)
    : Iterator(portal.GetIteratorBegin(),
	           static_cast<size_t>(portal.GetNumberOfValues())),
      NumberOfValues(portal.GetNumberOfValues())
  {  }

#endif // VTKM_MSVC

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorType GetBegin() const { return this->Iterator; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorType GetEnd() const {
    IteratorType iterator = this->Iterator;
	typedef typename std::iterator_traits<IteratorType>::difference_type
		difference_type;

#if !defined(VTKM_MSVC) || (defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL == 0)
    std::advance(iterator, static_cast<difference_type>(this->NumberOfValues));
#else
    //Visual Studio checked iterators throw exceptions when you try to advance
    //nullptr iterators even if the advancement length is zero. So instead
    //don't do the advancement at all
    if(this->NumberOfValues > 0)
    {
      std::advance(iterator, static_cast<difference_type>(this->NumberOfValues));
    }
#endif

    return iterator;
  }

private:
  IteratorType Iterator;
  vtkm::Id NumberOfValues;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_internal_ArrayPortalFromIterators_h
