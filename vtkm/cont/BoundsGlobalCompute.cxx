//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/BoundsGlobalCompute.h>

#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/FieldRangeGlobalCompute.h>
#include <vtkm/cont/PartitionedDataSet.h>

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
vtkm::Bounds BoundsGlobalCompute(const vtkm::cont::PartitionedDataSet& pds,
                                 vtkm::Id coordinate_system_index)
{
  return detail::MergeBoundsGlobal(vtkm::cont::BoundsCompute(pds, coordinate_system_index));
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::Bounds BoundsGlobalCompute(const vtkm::cont::DataSet& dataset, const std::string& name)
{
  return detail::MergeBoundsGlobal(vtkm::cont::BoundsCompute(dataset, name));
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::Bounds BoundsGlobalCompute(const vtkm::cont::PartitionedDataSet& pds, const std::string& name)
{
  return detail::MergeBoundsGlobal(vtkm::cont::BoundsCompute(pds, name));
}
}
}
