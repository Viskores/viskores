//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ArrayRangeCompute_h
#define vtk_m_cont_ArrayRangeCompute_h

#include <vtkm/Range.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleVirtualCoordinates.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

namespace vtkm
{
namespace cont
{

/// \brief Compute the range of the data in an array handle.
///
/// Given an \c ArrayHandle, this function computes the range (min and max) of
/// the values in the array. For arrays containing Vec values, the range is
/// computed for each component.
///
/// This method optionally takes a \c RuntimeDeviceTracker to control which
/// devices to try.
///
/// The result is returned in an \c ArrayHandle of \c Range objects. There is
/// one value in the returned array for every component of the input's value
/// type.
///
template <typename ArrayHandleType>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const ArrayHandleType& input,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker());

// Precompiled versions of ArrayRangeCompute
#define VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T(T, Storage)                                              \
  VTKM_CONT_EXPORT                                                                                 \
  VTKM_CONT                                                                                        \
  vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(                                          \
    const vtkm::cont::ArrayHandle<T, Storage>& input,                                              \
    vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())
#define VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(T, N, Storage)                                         \
  VTKM_CONT_EXPORT                                                                                 \
  VTKM_CONT                                                                                        \
  vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(                                          \
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, Storage>& input,                                \
    vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())

VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T(char, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Int8, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::UInt8, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Int16, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::UInt16, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Int32, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::UInt32, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Int64, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::UInt64, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Float32, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Float64, vtkm::cont::StorageTagBasic);

VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int32, 2, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int64, 2, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float32, 2, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float64, 2, vtkm::cont::StorageTagBasic);

VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int32, 3, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int64, 3, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float32, 3, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float64, 3, vtkm::cont::StorageTagBasic);

VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(char, 4, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int8, 4, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::UInt8, 4, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float32, 4, vtkm::cont::StorageTagBasic);
VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float64, 4, vtkm::cont::StorageTagBasic);

VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::FloatDefault,
                                    3,
                                    vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag);

VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::FloatDefault,
                                    3,
                                    vtkm::cont::ArrayHandleVirtualCoordinates::StorageTag);

#undef VTKM_ARRAY_RANGE_COMPUTE_EXPORT_T
#undef VTKM_ARRAY_RANGE_COMPUTE_EXPORT_VEC

// Implementation of composite vectors
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>,
                                typename vtkm::cont::ArrayHandleCompositeVector<
                                  vtkm::cont::ArrayHandle<vtkm::Float32>,
                                  vtkm::cont::ArrayHandle<vtkm::Float32>,
                                  vtkm::cont::ArrayHandle<vtkm::Float32>>::StorageTag>& input,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker());

VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>,
                                typename vtkm::cont::ArrayHandleCompositeVector<
                                  vtkm::cont::ArrayHandle<vtkm::Float64>,
                                  vtkm::cont::ArrayHandle<vtkm::Float64>,
                                  vtkm::cont::ArrayHandle<vtkm::Float64>>::StorageTag>& input,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker());

// Implementation of cartesian products
template <typename T, typename ArrayType1, typename ArrayType2, typename ArrayType3>
VTKM_CONT inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<
    T,
    vtkm::cont::internal::StorageTagCartesianProduct<ArrayType1, ArrayType2, ArrayType3>>& input,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())
{
  vtkm::cont::ArrayHandle<vtkm::Range> result;
  result.Allocate(3);

  vtkm::cont::ArrayHandle<vtkm::Range> componentRangeArray;
  vtkm::Range componentRange;

  ArrayType1 firstArray = input.GetStorage().GetFirstArray();
  componentRangeArray = vtkm::cont::ArrayRangeCompute(firstArray, tracker);
  componentRange = componentRangeArray.GetPortalConstControl().Get(0);
  result.GetPortalControl().Set(0, componentRange);

  ArrayType2 secondArray = input.GetStorage().GetSecondArray();
  componentRangeArray = vtkm::cont::ArrayRangeCompute(secondArray, tracker);
  componentRange = componentRangeArray.GetPortalConstControl().Get(0);
  result.GetPortalControl().Set(1, componentRange);

  ArrayType3 thirdArray = input.GetStorage().GetThirdArray();
  componentRangeArray = vtkm::cont::ArrayRangeCompute(thirdArray, tracker);
  componentRange = componentRangeArray.GetPortalConstControl().Get(0);
  result.GetPortalControl().Set(2, componentRange);

  return result;
}

VTKM_CONT_EXPORT void ThrowArrayRangeComputeFailed();
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayRangeCompute_h
