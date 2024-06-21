//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ConvertNumComponentsToOffsets_h
#define vtk_m_cont_ConvertNumComponentsToOffsets_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{


/// `ConvertNumComponentsToOffsets` takes an array of Vec sizes (i.e. the number of components in
/// each `Vec`) and returns an array of offsets to a packed array of such `Vec`s. The resulting
/// array can be used with `ArrayHandleGroupVecVariable`.
///
/// @param[in] numComponentsArray the input array that specifies the number of components in each group
/// Vec.
///
/// @param[out] offsetsArray (optional) the output \c ArrayHandle, which must have a value type of \c
/// vtkm::Id. If the output \c ArrayHandle is not given, it is returned.
///
/// @param[in] componentsArraySize (optional) a reference to a \c vtkm::Id and is filled with the
/// expected size of the component values array.
///
/// @param[in] device (optional) specifies the device on which to run the conversion.
///
/// Note that this function is pre-compiled for some set of `ArrayHandle` types. If you get a
/// warning about an inefficient conversion (or the operation fails outright), you might need to
/// use `vtkm::cont::internal::ConvertNumComponentsToOffsetsTemplate`.
///
VTKM_CONT_EXPORT void ConvertNumComponentsToOffsets(
  const vtkm::cont::UnknownArrayHandle& numComponentsArray,
  vtkm::cont::ArrayHandle<vtkm::Id>& offsetsArray,
  vtkm::Id& componentsArraySize,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{});

VTKM_CONT_EXPORT void ConvertNumComponentsToOffsets(
  const vtkm::cont::UnknownArrayHandle& numComponentsArray,
  vtkm::cont::ArrayHandle<vtkm::Id>& offsetsArray,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{});

VTKM_CONT_EXPORT vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumComponentsToOffsets(
  const vtkm::cont::UnknownArrayHandle& numComponentsArray,
  vtkm::Id& componentsArraySize,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{});

VTKM_CONT_EXPORT vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumComponentsToOffsets(
  const vtkm::cont::UnknownArrayHandle& numComponentsArray,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{});

} // namespace vtkm::cont
} // namespace vtkm

#endif // vtk_m_cont_ConvertNumComponentsToOffsets_h
