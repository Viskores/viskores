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
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/ArrayHandleStride.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleXGCCoordinates.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/UnknownArrayHandle.h>

namespace vtkm
{
namespace cont
{

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
/// for arbitrary `ArrayHandle`s not in this precompiled list, you need to use
/// `ArrayRangeComputeTemplate` (declared in `ArrayRangeComputeTemplate`), which
/// will compile for any `ArrayHandle` type not already handled.
///
VTKM_CONT_EXPORT vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::UnknownArrayHandle& array,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{});

namespace internal
{

VTKM_CONT_EXPORT void ThrowArrayRangeComputeFailed();

} // namespace internal

VTKM_DEPRECATED(2.1, "Moved to vtkm::cont::internal.")
inline void ThrowArrayRangeComputeFailed()
{
  internal::ThrowArrayRangeComputeFailed();
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayRangeCompute_h
