//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/cont/BoundsCompute.h>

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/MultiBlock.h>

#include <numeric> // for std::accumulate

namespace vtkm
{
namespace cont
{

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::Bounds BoundsCompute(const vtkm::cont::DataSet& dataset, vtkm::Id coordinate_system_index)
{
  return dataset.GetNumberOfCoordinateSystems() > coordinate_system_index
    ? dataset.GetCoordinateSystem(coordinate_system_index).GetBounds()
    : vtkm::Bounds();
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::Bounds BoundsCompute(const vtkm::cont::MultiBlock& multiblock,
                           vtkm::Id coordinate_system_index)
{
  return std::accumulate(multiblock.begin(),
                         multiblock.end(),
                         vtkm::Bounds(),
                         [=](const vtkm::Bounds& val, const vtkm::cont::DataSet& block) {
                           return val + vtkm::cont::BoundsCompute(block, coordinate_system_index);
                         });
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::Bounds BoundsCompute(const vtkm::cont::DataSet& dataset, const std::string& name)
{
  try
  {
    return dataset.GetCoordinateSystem(name).GetBounds();
  }
  catch (vtkm::cont::ErrorBadValue&)
  {
    // missing coordinate_system_index, return empty bounds.
    return vtkm::Bounds();
  }
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::Bounds BoundsCompute(const vtkm::cont::MultiBlock& multiblock, const std::string& name)
{
  return std::accumulate(multiblock.begin(),
                         multiblock.end(),
                         vtkm::Bounds(),
                         [=](const vtkm::Bounds& val, const vtkm::cont::DataSet& block) {
                           return val + vtkm::cont::BoundsCompute(block, name);
                         });
}
}
}
