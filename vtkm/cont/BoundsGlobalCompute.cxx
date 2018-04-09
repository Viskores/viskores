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
#include <vtkm/cont/BoundsGlobalCompute.h>

#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/FieldRangeGlobalCompute.h>
#include <vtkm/cont/MultiBlock.h>

#include <numeric> // for std::accumulate

namespace vtkm
{
namespace cont
{

namespace detail
{
VTKM_CONT
vtkm::Bounds MergeBoundsGlobal(const vtkm::Bounds& local)
{
  vtkm::cont::ArrayHandle<vtkm::Range> ranges;
  ranges.Allocate(3);
  ranges.GetPortalControl().Set(0, local.X);
  ranges.GetPortalControl().Set(1, local.Y);
  ranges.GetPortalControl().Set(2, local.Z);

  ranges = vtkm::cont::detail::MergeRangesGlobal(ranges);
  auto portal = ranges.GetPortalConstControl();
  return vtkm::Bounds(portal.Get(0), portal.Get(1), portal.Get(2));
}
}


//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::Bounds BoundsGlobalCompute(const vtkm::cont::DataSet& dataset,
                                 vtkm::Id coordinate_system_index)
{
  return detail::MergeBoundsGlobal(vtkm::cont::BoundsCompute(dataset, coordinate_system_index));
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::Bounds BoundsGlobalCompute(const vtkm::cont::MultiBlock& multiblock,
                                 vtkm::Id coordinate_system_index)
{
  return detail::MergeBoundsGlobal(vtkm::cont::BoundsCompute(multiblock, coordinate_system_index));
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::Bounds BoundsGlobalCompute(const vtkm::cont::DataSet& dataset, const std::string& name)
{
  return detail::MergeBoundsGlobal(vtkm::cont::BoundsCompute(dataset, name));
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::Bounds BoundsGlobalCompute(const vtkm::cont::MultiBlock& multiblock, const std::string& name)
{
  return detail::MergeBoundsGlobal(vtkm::cont::BoundsCompute(multiblock, name));
}
}
}
