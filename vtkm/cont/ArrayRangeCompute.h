//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayRangeCompute_h
#define vtk_m_cont_ArrayRangeCompute_h

#include <vtkm/Range.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleVirtualCoordinates.h>
#include <vtkm/cont/DeviceAdapterTag.h>

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
/// This method optionally takes a \c vtkm::cont::DeviceAdapterId to control which
/// devices to try.
///
/// The result is returned in an \c ArrayHandle of \c Range objects. There is
/// one value in the returned array for every component of the input's value
/// type.
///
template <typename ArrayHandleType>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const ArrayHandleType& input,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny());

// Precompiled versions of ArrayRangeCompute
#define VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(T, Storage)                                             \
  VTKM_CONT_EXPORT                                                                                 \
  VTKM_CONT                                                                                        \
  vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(                                          \
    const vtkm::cont::ArrayHandle<T, Storage>& input,                                              \
    vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
#define VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(T, N, Storage)                                        \
  VTKM_CONT_EXPORT                                                                                 \
  VTKM_CONT                                                                                        \
  vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(                                          \
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, Storage>& input,                                \
    vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())

VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(char, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Int8, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::UInt8, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Int16, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::UInt16, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Int32, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::UInt32, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Int64, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::UInt64, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Float32, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Float64, vtkm::cont::StorageTagBasic);

VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int32, 2, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int64, 2, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float32, 2, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float64, 2, vtkm::cont::StorageTagBasic);

VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int32, 3, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int64, 3, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float32, 3, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float64, 3, vtkm::cont::StorageTagBasic);

VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(char, 4, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int8, 4, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::UInt8, 4, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float32, 4, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float64, 4, vtkm::cont::StorageTagBasic);

#undef VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T
#undef VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC

VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandleVirtual<vtkm::Vec3f>& input,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny());

VTKM_CONT_EXPORT VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<vtkm::Vec3f,
                                vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag>& array,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny());

// Implementation of composite vectors
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<vtkm::Vec3f_32,
                                typename vtkm::cont::ArrayHandleCompositeVector<
                                  vtkm::cont::ArrayHandle<vtkm::Float32>,
                                  vtkm::cont::ArrayHandle<vtkm::Float32>,
                                  vtkm::cont::ArrayHandle<vtkm::Float32>>::StorageTag>& input,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny());

VTKM_CONT_EXPORT VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<vtkm::Vec3f_64,
                                typename vtkm::cont::ArrayHandleCompositeVector<
                                  vtkm::cont::ArrayHandle<vtkm::Float64>,
                                  vtkm::cont::ArrayHandle<vtkm::Float64>,
                                  vtkm::cont::ArrayHandle<vtkm::Float64>>::StorageTag>& input,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny());

// Implementation of cartesian products
template <typename T, typename ST1, typename ST2, typename ST3>
VTKM_CONT inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>,
                                vtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>& input,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
{
  vtkm::cont::ArrayHandle<vtkm::Range> result;
  result.Allocate(3);

  vtkm::cont::ArrayHandle<vtkm::Range> componentRangeArray;
  vtkm::Range componentRange;

  vtkm::cont::ArrayHandle<T, ST1> firstArray = input.GetStorage().GetFirstArray();
  componentRangeArray = vtkm::cont::ArrayRangeCompute(firstArray, device);
  componentRange = componentRangeArray.GetPortalConstControl().Get(0);
  result.GetPortalControl().Set(0, componentRange);

  vtkm::cont::ArrayHandle<T, ST2> secondArray = input.GetStorage().GetSecondArray();
  componentRangeArray = vtkm::cont::ArrayRangeCompute(secondArray, device);
  componentRange = componentRangeArray.GetPortalConstControl().Get(0);
  result.GetPortalControl().Set(1, componentRange);

  vtkm::cont::ArrayHandle<T, ST3> thirdArray = input.GetStorage().GetThirdArray();
  componentRangeArray = vtkm::cont::ArrayRangeCompute(thirdArray, device);
  componentRange = componentRangeArray.GetPortalConstControl().Get(0);
  result.GetPortalControl().Set(2, componentRange);

  return result;
}

VTKM_CONT_EXPORT void ThrowArrayRangeComputeFailed();
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayRangeCompute_h
