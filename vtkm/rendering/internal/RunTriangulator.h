//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_internal_RunTriangulator_h
#define vtk_m_rendering_internal_RunTriangulator_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

namespace vtkm
{
namespace rendering
{
namespace internal
{

/// This is a wrapper around the Triangulator worklet so that the
/// implementation of the triangulator only gets compiled once. This function
/// really is a stop-gap. Eventually, the Triangulator should be moved to
/// filters, and filters should be compiled in a library (for the same reason).
///
VTKM_RENDERING_EXPORT
void RunTriangulator(const vtkm::cont::DynamicCellSet& cellSet,
                     vtkm::cont::ArrayHandle<vtkm::Id4>& indices,
                     vtkm::Id& numberOfTriangles);
}
}
} // namespace vtkm::rendering::internal

#endif //vtk_m_rendering_internal_RunTriangulator_h
