//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
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

namespace vtkm
{
namespace cont
{

namespace internal
{

/// \brief An implicit array portal that returns an counting value.
template <class CountingValueType>
class VTKM_ALWAYS_EXPORT ArrayPortalCounting
{
  using ComponentType = typename vtkm::VecTraits<CountingValueType>::ComponentType;

public:
  using ValueType = CountingValueType;

  VTKM_EXEC_CONT
  ArrayPortalCounting()
    : Start(0)
    , Step(1)
    , NumberOfValues(0)
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalCounting(ValueType start, ValueType step, vtkm::Id numValues)
    : Start(start)
    , Step(step)
    , NumberOfValues(numValues)
  {
  }

  template <typename OtherValueType>
  VTKM_EXEC_CONT ArrayPortalCounting(const ArrayPortalCounting<OtherValueType>& src)
    : Start(src.Start)
    , Step(src.Step)
    , NumberOfValues(src.NumberOfValues)
  {
  }

  template <typename OtherValueType>
  VTKM_EXEC_CONT ArrayPortalCounting<ValueType>& operator=(
    const ArrayPortalCounting<OtherValueType>& src)
  {
    this->Start = src.Start;
    this->Step = src.Step;
    this->NumberOfValues = src.NumberOfValues;
    return *this;
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    return ValueType(this->Start + this->Step * ValueType(static_cast<ComponentType>(index)));
  }

  VTKM_EXEC_CONT
  void Set(vtkm::Id vtkmNotUsed(index), const ValueType& vtkmNotUsed(value)) const
  {
    VTKM_ASSERT(false && "Cannot write to read-only counting array.");
  }

private:
  ValueType Start;
  ValueType Step;
  vtkm::Id NumberOfValues;
};

/// A convenience class that provides a typedef to the appropriate tag for
/// a counting storage.
template <typename ConstantValueType>
struct ArrayHandleCountingTraits
{
  using Tag =
    vtkm::cont::StorageTagImplicit<vtkm::cont::internal::ArrayPortalCounting<ConstantValueType>>;
};

} // namespace internal

/// ArrayHandleCounting is a specialization of ArrayHandle. By default it
/// contains a increment value, that is increment for each step between zero
/// and the passed in length
template <typename CountingValueType>
class ArrayHandleCounting : public vtkm::cont::ArrayHandle<
                              CountingValueType,
                              typename internal::ArrayHandleCountingTraits<CountingValueType>::Tag>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleCounting,
    (ArrayHandleCounting<CountingValueType>),
    (vtkm::cont::ArrayHandle<
      CountingValueType,
      typename internal::ArrayHandleCountingTraits<CountingValueType>::Tag>));

  VTKM_CONT
  ArrayHandleCounting(CountingValueType start, CountingValueType step, vtkm::Id length)
    : Superclass(typename Superclass::PortalConstControl(start, step, length))
  {
  }
};

/// A convenience function for creating an ArrayHandleCounting. It takes the
/// value to start counting from and and the number of times to increment.
template <typename CountingValueType>
VTKM_CONT vtkm::cont::ArrayHandleCounting<CountingValueType>
make_ArrayHandleCounting(CountingValueType start, CountingValueType step, vtkm::Id length)
{
  return vtkm::cont::ArrayHandleCounting<CountingValueType>(start, step, length);
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleCounting_h
