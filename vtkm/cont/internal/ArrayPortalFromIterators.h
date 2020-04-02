//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename IteratorT, typename Enable = void>
class ArrayPortalFromIterators;

/// This templated implementation of an ArrayPortal allows you to adapt a pair
/// of begin/end iterators to an ArrayPortal interface.
///
template <class IteratorT>
class ArrayPortalFromIterators<IteratorT,
                               typename std::enable_if<!std::is_const<
                                 typename std::remove_pointer<IteratorT>::type>::value>::type>
{
public:
  using ValueType = typename std::iterator_traits<IteratorT>::value_type;
  using IteratorType = IteratorT;

  ArrayPortalFromIterators() = default;

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
  template <class OtherIteratorT>
  VTKM_EXEC_CONT ArrayPortalFromIterators(const ArrayPortalFromIterators<OtherIteratorT>& src)
    : BeginIterator(src.GetIteratorBegin())
    , NumberOfValues(src.GetNumberOfValues())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return *this->IteratorAt(index); }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const { *(this->BeginIterator + index) = value; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorT GetIteratorBegin() const { return this->BeginIterator; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorT GetIteratorEnd() const
  {
    IteratorType iterator = this->BeginIterator;
    using difference_type = typename std::iterator_traits<IteratorType>::difference_type;
    iterator += static_cast<difference_type>(this->NumberOfValues);
    return iterator;
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

template <class IteratorT>
class ArrayPortalFromIterators<IteratorT,
                               typename std::enable_if<std::is_const<
                                 typename std::remove_pointer<IteratorT>::type>::value>::type>
{
public:
  using ValueType = typename std::iterator_traits<IteratorT>::value_type;
  using IteratorType = IteratorT;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalFromIterators()
    : BeginIterator(nullptr)
    , NumberOfValues(0)
  {
  }

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
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <class OtherIteratorT>
  VTKM_EXEC_CONT ArrayPortalFromIterators(const ArrayPortalFromIterators<OtherIteratorT>& src)
    : BeginIterator(src.GetIteratorBegin())
    , NumberOfValues(src.GetNumberOfValues())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return *this->IteratorAt(index); }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void Set(vtkm::Id vtkmNotUsed(index), const ValueType& vtkmNotUsed(value)) const
  {
#if !(defined(VTKM_MSVC) && defined(VTKM_CUDA))
    VTKM_ASSERT(false && "Attempted to write to constant array.");
#endif
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorT GetIteratorBegin() const { return this->BeginIterator; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorT GetIteratorEnd() const
  {
    using difference_type = typename std::iterator_traits<IteratorType>::difference_type;
    IteratorType iterator = this->BeginIterator;
    iterator += static_cast<difference_type>(this->NumberOfValues);
    return iterator;
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
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ArrayPortalFromIterators_h
