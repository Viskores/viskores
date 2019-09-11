//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/internal/RunTriangulator.h>

#include <vtkm/cont/TryExecute.h>
#include <vtkm/rendering/Triangulator.h>

namespace vtkm
{
namespace rendering
{
namespace internal
{

void RunTriangulator(const vtkm::cont::DynamicCellSet& cellSet,
                     vtkm::cont::ArrayHandle<vtkm::Id4>& indices,
                     vtkm::Id& numberOfTriangles)
{
  vtkm::rendering::Triangulator triangulator;
  triangulator.Run(cellSet, indices, numberOfTriangles);
}
}
}
} // namespace vtkm::rendering::internal
