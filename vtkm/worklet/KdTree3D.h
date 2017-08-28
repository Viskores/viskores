//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_m_worklet_KdTree3D_h
#define vtkm_m_worklet_KdTree3D_h

#include <vtkm/worklet/spatialstructure/KdTree3DConstruction.h>
#include <vtkm/worklet/spatialstructure/KdTree3DNNSearch.h>

namespace vtkm
{
namespace worklet
{

class KdTree3D
{
public:
  KdTree3D() = default;

  template <typename CoordType, typename CoordStorageTag, typename DeviceAdapter>
  void Build(const vtkm::cont::ArrayHandle<vtkm::Vec<CoordType, 3>, CoordStorageTag>& coords,
             DeviceAdapter device)
  {
    vtkm::worklet::spatialstructure::KdTree3DConstruction().Run(
      coords, this->PointIds, this->SplitIds, device);
  }

  template <typename CoordType,
            typename CoordStorageTag1,
            typename CoordStorageTag2,
            typename DeviceAdapter>
  void Run(const vtkm::cont::ArrayHandle<vtkm::Vec<CoordType, 3>, CoordStorageTag1>& coords,
           const vtkm::cont::ArrayHandle<vtkm::Vec<CoordType, 3>, CoordStorageTag2>& queryPoints,
           vtkm::cont::ArrayHandle<vtkm::Id>& nearestNeighborIds,
           vtkm::cont::ArrayHandle<CoordType>& distances,
           DeviceAdapter device)
  {
    vtkm::worklet::spatialstructure::KdTree3DNNSearch().Run(
      coords, this->PointIds, this->SplitIds, queryPoints, nearestNeighborIds, distances, device);
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> PointIds;
  vtkm::cont::ArrayHandle<vtkm::Id> SplitIds;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Kdtree3D_h
