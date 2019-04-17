//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
  /// \param coords An ArrayHandle of x, y, z coordinates of input points.
  ///
  template <typename CoordType, typename CoordStorageTag>
  void Build(const vtkm::cont::ArrayHandle<vtkm::Vec<CoordType, 3>, CoordStorageTag>& coords)
  {
    vtkm::worklet::spatialstructure::KdTree3DConstruction().Run(
      coords, this->PointIds, this->SplitIds);
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
           DeviceAdapter deviceId)
  {
    vtkm::worklet::spatialstructure::KdTree3DNNSearch().Run(
      coords, this->PointIds, this->SplitIds, queryPoints, nearestNeighborIds, distances, deviceId);
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> PointIds;
  vtkm::cont::ArrayHandle<vtkm::Id> SplitIds;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Kdtree3D_h
