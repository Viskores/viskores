//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ArrayRangeComputeUtils_h
#define vtk_m_cont_internal_ArrayRangeComputeUtils_h

#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

VTKM_CONT_EXPORT vtkm::Id2 GetFirstAndLastUnmaskedIndices(
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{});

}
}
} // vtkm::cont::internal

#endif // vtk_m_cont_internal_ArrayRangeComputeUtils_h
