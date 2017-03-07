//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ArrayRangeCompute_h
#define vtk_m_cont_ArrayRangeCompute_h

#include <vtkm/BinaryOperators.h>
#include <vtkm/Range.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm {
namespace cont {

namespace internal {

struct RangeMin
{
  template<typename T>
  VTKM_EXEC
  T operator()(const T& a, const T& b)const { return vtkm::Min(a,b); }
};

struct RangeMax
{
  template<typename T>
  VTKM_EXEC
  T operator()(const T& a, const T& b)const { return vtkm::Max(a,b); }
};

} // namespace internal

/// \brief Compute the range of the data in an array handle.
///
/// Given an \c ArrayHandle, this function computes the range (min and max) of
/// the values in the array. For arrays containing Vec values, the range is
/// computed for each component.
///
/// This method also takes a device adapter tag to specify the device on which
/// to compute the range.
///
/// The result is returned in an \c ArrayHandle of \c Range objects. There is
/// one value in the returned array for every component of the input's value
/// type.
///
template<typename ArrayHandleType, typename Device>
inline
vtkm::cont::ArrayHandle<vtkm::Range>
ArrayRangeCompute(const ArrayHandleType &input, Device)
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  typedef typename ArrayHandleType::ValueType ValueType;
  typedef vtkm::VecTraits<ValueType> VecType;
  const vtkm::IdComponent NumberOfComponents = VecType::NUM_COMPONENTS;

  typedef vtkm::cont::DeviceAdapterAlgorithm<Device> Algorithm;

  //not the greatest way of doing this for performance reasons. But
  //this implementation should generate the smallest amount of code
  vtkm::Vec<ValueType,2> initial(input.GetPortalConstControl().Get(0));

  vtkm::Vec<ValueType, 2> result =
      Algorithm::Reduce(input, initial, vtkm::MinAndMax<ValueType>());

  vtkm::cont::ArrayHandle<vtkm::Range> rangeArray;
  rangeArray.Allocate(NumberOfComponents);
  for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
  {
    rangeArray.GetPortalControl().Set(
          i, vtkm::Range(VecType::GetComponent(result[0], i),
                         VecType::GetComponent(result[1], i)));
  }

  return rangeArray;
}

// Special implementation for regular point coordinates, which are easy
// to determine.
template<typename Device>
inline
vtkm::cont::ArrayHandle<vtkm::Range>
ArrayRangeCompute(const vtkm::cont::ArrayHandle<
                    vtkm::Vec<vtkm::FloatDefault,3>,
                    vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag>
                  &array,
                  Device)
{
  vtkm::internal::ArrayPortalUniformPointCoordinates portal =
      array.GetPortalConstControl();

  // In this portal we know that the min value is the first entry and the
  // max value is the last entry.
  vtkm::Vec<vtkm::FloatDefault,3> minimum = portal.Get(0);
  vtkm::Vec<vtkm::FloatDefault,3> maximum =
      portal.Get(portal.GetNumberOfValues()-1);

  vtkm::cont::ArrayHandle<vtkm::Range> rangeArray;
  rangeArray.Allocate(3);
  vtkm::cont::ArrayHandle<vtkm::Range>::PortalControl outPortal =
      rangeArray.GetPortalControl();
  outPortal.Set(0, vtkm::Range(minimum[0], maximum[0]));
  outPortal.Set(1, vtkm::Range(minimum[1], maximum[1]));
  outPortal.Set(2, vtkm::Range(minimum[2], maximum[2]));

  return rangeArray;
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayRangeCompute_h
