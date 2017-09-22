//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
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

  /// \brief Construct a 3D KD-tree for 3D point positions.
  ///
  /// \tparam CoordType type of the x, y, z component of the point coordinates.
  /// \tparam CoordStorageTag
  /// \tparam DeviceAdapter
  /// \param coords An ArrayHandle of x, y, z coordinates of input points.
  /// \param device Tag for selecting device adapter.
  ///
  template <typename CoordType, typename CoordStorageTag, typename DeviceAdapter>
  void Build(const vtkm::cont::ArrayHandle<vtkm::Vec<CoordType, 3>, CoordStorageTag>& coords,
             DeviceAdapter device)
  {
    vtkm::worklet::spatialstructure::KdTree3DConstruction().Run(
      coords, this->PointIds, this->SplitIds, device);
  }

  /// \brief Nearest neighbor search using KD-Tree
  ///
  /// Parallel search of nearest neighbor for each point in the \c queryPoints in the the set of
  /// \c coords. Returns nearest neighbor in \c nearestNeighborId and distance to nearest neighbor
  /// in \c distances.
  ///
  /// \tparam CoordType
  /// \tparam CoordStorageTag1
  /// \tparam CoordStorageTag2
  /// \tparam DeviceAdapter
  /// \param coords Point coordinates for training data set (haystack)
  /// \param queryPoints Point coordinates to query for nearest neighbor (needles).
  /// \param nearestNeighborIds Nearest neighbor in the traning data set for each points in the
  ///                           testing set
  /// \param distances Distances between query points and their nearest neighbors.
  /// \param device Tag for selecting device adapter.
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
