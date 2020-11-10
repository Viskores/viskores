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
#include <vtkm/cont/DeviceAdapterTag.h>

namespace vtkm
{
namespace cont
{

///@{
/// \brief Compute the range of the data in an array handle.
///
/// Given an `ArrayHandle`, this function computes the range (min and max) of
/// the values in the array. For arrays containing Vec values, the range is
/// computed for each component.
///
/// This method optionally takes a `vtkm::cont::DeviceAdapterId` to control which
/// devices to try.
///
/// The result is returned in an `ArrayHandle` of `Range` objects. There is
/// one value in the returned array for every component of the input's value
/// type.
///
/// Note that the ArrayRangeCompute.h header file contains only precompiled overloads
/// of ArrayRangeCompute. This is so that ArrayRangeCompute.h can be included in
/// code that does not use a device compiler. If you need to compute array ranges
/// for arbitrary `ArrayHandle`s not in this precompiled list, you need to include
/// ArrayRangeComputeTemplate.h. This contains a templated version of ArrayRangeCompute
/// that will compile for any `ArrayHandle` type not already handled.
///

#define VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(T, Storage)    \
  VTKM_CONT_EXPORT                                        \
  VTKM_CONT                                               \
  vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute( \
    const vtkm::cont::ArrayHandle<T, Storage>& input,     \
    vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
#define VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(T, N, Storage)         \
  VTKM_CONT_EXPORT                                                  \
  VTKM_CONT                                                         \
  vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, Storage>& input, \
    vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())

#define VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_ALL_SCALAR_T(Storage) \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(char, Storage);           \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Int8, Storage);     \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::UInt8, Storage);    \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Int16, Storage);    \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::UInt16, Storage);   \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Int32, Storage);    \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::UInt32, Storage);   \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Int64, Storage);    \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::UInt64, Storage);   \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Float32, Storage);  \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(vtkm::Float64, Storage)

#define VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_ALL_VEC(N, Storage)       \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(char, N, Storage);          \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int8, N, Storage);    \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::UInt8, N, Storage);   \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int16, N, Storage);   \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::UInt16, N, Storage);  \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int32, N, Storage);   \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::UInt32, N, Storage);  \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Int64, N, Storage);   \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::UInt64, N, Storage);  \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float32, N, Storage); \
  VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(vtkm::Float64, N, Storage)

VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_ALL_SCALAR_T(vtkm::cont::StorageTagBasic);

VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_ALL_VEC(2, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_ALL_VEC(3, vtkm::cont::StorageTagBasic);
VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_ALL_VEC(4, vtkm::cont::StorageTagBasic);

#undef VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T
#undef VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC
#undef VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_ALL_SCALAR_T
#undef VTK_M_ARRAY_RANGE_COMPUTE_EXPORT_ALL_VEC

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
  componentRange = componentRangeArray.ReadPortal().Get(0);
  result.WritePortal().Set(0, componentRange);

  vtkm::cont::ArrayHandle<T, ST2> secondArray = input.GetStorage().GetSecondArray();
  componentRangeArray = vtkm::cont::ArrayRangeCompute(secondArray, device);
  componentRange = componentRangeArray.ReadPortal().Get(0);
  result.WritePortal().Set(1, componentRange);

  vtkm::cont::ArrayHandle<T, ST3> thirdArray = input.GetStorage().GetThirdArray();
  componentRangeArray = vtkm::cont::ArrayRangeCompute(thirdArray, device);
  componentRange = componentRangeArray.ReadPortal().Get(0);
  result.WritePortal().Set(2, componentRange);

  return result;
}
///@}

VTKM_CONT_EXPORT void ThrowArrayRangeComputeFailed();
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayRangeCompute_h
