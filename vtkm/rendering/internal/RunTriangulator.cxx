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
                     vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>& indices,
                     vtkm::Id& numberOfTriangles)
{
  vtkm::rendering::Triangulator triangulator;
  triangulator.Run(cellSet, indices, numberOfTriangles);
}
}
}
} // namespace vtkm::rendering::internal
