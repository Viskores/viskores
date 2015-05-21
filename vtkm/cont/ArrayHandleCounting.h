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
#ifndef vtk_m_cont_ArrayHandleCounting_h
#define vtk_m_cont_ArrayHandleCounting_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageImplicit.h>

#include <vtkm/VecTraits.h>

namespace vtkm {
namespace cont {

namespace internal {

/// \brief An implicit array portal that returns an counting value.
template <class CountingValueType>
class ArrayPortalCounting
{
  typedef typename vtkm::VecTraits<CountingValueType>::ComponentType
      ComponentType;

public:
  typedef CountingValueType ValueType;

  typedef vtkm::cont::internal::IteratorFromArrayPortal<
          ArrayPortalCounting < ValueType> > IteratorType;

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalCounting() :
    StartingValue(),
    NumberOfValues(0)
  {  }

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalCounting(ValueType startingValue, vtkm::Id numValues) :
    StartingValue(startingValue),
    NumberOfValues(numValues)
  {  }

  template<typename OtherValueType>
  VTKM_EXEC_CONT_EXPORT
  ArrayPortalCounting(const ArrayPortalCounting<OtherValueType> &src)
    : StartingValue(src.StartingValue),
      NumberOfValues(src.NumberOfValues)
  {  }

  template<typename OtherValueType>
  VTKM_EXEC_CONT_EXPORT
  ArrayPortalCounting<ValueType> &operator=(
    const ArrayPortalCounting<OtherValueType> &src)
  {
    this->StartingValue = src.StartingValue;
    this->NumberOfValues = src.NumberOfValues;
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_EXEC_CONT_EXPORT
  ValueType Get(vtkm::Id index) const {
    return StartingValue + ValueType(static_cast<ComponentType>(index));
  }

private:
  ValueType StartingValue;
  vtkm::Id NumberOfValues;
};

/// A convenience class that provides a typedef to the appropriate tag for
/// a counting storage.
template<typename ConstantValueType>
struct ArrayHandleCountingTraits
{
  typedef vtkm::cont::StorageTagImplicit<
      vtkm::cont::internal::ArrayPortalCounting<ConstantValueType> > Tag;
};

} // namespace internal

/// ArrayHandleCounting is a specialization of ArrayHandle. By default it
/// contains a increment value, that is increment for each step between zero
/// and the passed in length
template <typename CountingValueType>
class ArrayHandleCounting
    : public vtkm::cont::ArrayHandle <
          CountingValueType,
          typename internal::ArrayHandleCountingTraits<CountingValueType>::Tag
          >
{
  typedef vtkm::cont::ArrayHandle <
          CountingValueType,
          typename internal::ArrayHandleCountingTraits<CountingValueType>::Tag
          > Superclass;
public:

  VTKM_CONT_EXPORT
  ArrayHandleCounting(CountingValueType startingValue, vtkm::Id length)
    :Superclass(typename Superclass::PortalConstControl(startingValue, length))
  {
  }

  VTKM_CONT_EXPORT
  ArrayHandleCounting():Superclass() {}
};

/// A convenience function for creating an ArrayHandleCounting. It takes the
/// value to start counting from and and the number of times to increment.
template<typename CountingValueType>
VTKM_CONT_EXPORT
vtkm::cont::ArrayHandleCounting<CountingValueType>
make_ArrayHandleCounting(CountingValueType startingValue,
                         vtkm::Id length)
{
  return vtkm::cont::ArrayHandleCounting<CountingValueType>(startingValue,
                                                            length);
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleCounting_h
