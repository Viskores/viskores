//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_PointLocatorUniformGrid_h
#define vtk_m_exec_PointLocatorUniformGrid_h

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/exec/PointLocator.h>

namespace vtkm
{
namespace exec
{

// TODO: remove template T
template <typename DeviceAdapter>
class PointLocatorUniformGrid : public vtkm::exec::PointLocator
{
public:
  // TODO: figure hout how to parametize/passing DeviceAdapter.
  //using DeviceAdapter = vtkm::cont::DeviceAdapterTagSerial;
  using CoordPortalType = typename vtkm::cont::ArrayHandle<
    vtkm::Vec<vtkm::FloatDefault, 3>>::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using IdPortalType =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapter>::PortalConst;


  // TODO: should constructor be VTKM_CONT or VTKM_EXEC?
  VTKM_CONT
  PointLocatorUniformGrid() = default;

  VTKM_CONT
  PointLocatorUniformGrid(const vtkm::Vec<vtkm::FloatDefault, 3>& _min,
                          const vtkm::Vec<vtkm::FloatDefault, 3>& _max,
                          const vtkm::Vec<vtkm::Id, 3>& _dims,
                          const CoordPortalType& _coords,
                          const IdPortalType& _pointIds,
                          const IdPortalType& _cellLower,
                          const IdPortalType& _cellUpper)
    : Min(_min)
    , Dims(_dims)
    , Dxdydz((_max - Min) / Dims)
    , coords(_coords)
    , pointIds(_pointIds)
    , cellLower(_cellLower)
    , cellUpper(_cellUpper)
  {
  }

  /// \brief Nearest neighbor search using a Uniform Grid
  ///
  /// Parallel search of nearesat neighbor for each point in the \c queryPoints in the set of
  /// \c coords. Returns neareast neighbot in \c nearestNeighborIds and distances to nearest
  /// neighbor in \c distances.
  ///
  /// \param queryPoint Point coordinates to query for nearest neighbor.
  /// \param nearestNeighborId Neareast neighbor in the training dataset for each points in
  ///                            the test set
  /// \param distance Distance between query points and their nearest neighbors.
  VTKM_EXEC virtual void FindNearestNeighbor(vtkm::Vec<vtkm::FloatDefault, 3> queryPoint,
                                             vtkm::Id& nearestNeighborId,
                                             FloatDefault& distance) const override
  {
//std::cout << "FindNeareastNeighbor: " << queryPoint << std::endl;
#if 1
    auto nlayers = vtkm::Max(vtkm::Max(Dims[0], Dims[1]), Dims[2]);

    vtkm::Vec<vtkm::Id, 3> xyz = (queryPoint - Min) / Dxdydz;

    float min_distance = std::numeric_limits<float>::max();
    vtkm::Id neareast = -1;

    for (vtkm::Id layer = 0; layer < nlayers; layer++)
    {
      vtkm::Id minx = vtkm::Max(vtkm::Id(), xyz[0] - layer);
      vtkm::Id maxx = vtkm::Min(Dims[0] - 1, xyz[0] + layer);
      vtkm::Id miny = vtkm::Max(vtkm::Id(), xyz[1] - layer);
      vtkm::Id maxy = vtkm::Min(Dims[1] - 1, xyz[1] + layer);
      vtkm::Id minz = vtkm::Max(vtkm::Id(), xyz[2] - layer);
      vtkm::Id maxz = vtkm::Min(Dims[2] - 1, xyz[2] + layer);

      for (auto i = minx; i <= maxx; i++)
      {
        for (auto j = miny; j <= maxy; j++)
        {
          for (auto k = minz; k <= maxz; k++)
          {
            if (i == (xyz[0] + layer) || i == (xyz[0] - layer) || j == (xyz[1] + layer) ||
                j == (xyz[1] - layer) || k == (xyz[2] + layer) || k == (xyz[2] - layer))
            {
              auto cellid = i + j * Dims[0] + k * Dims[0] * Dims[1];
              auto lower = cellLower.Get(cellid);
              auto upper = cellUpper.Get(cellid);
              for (auto index = lower; index < upper; index++)
              {
                auto pointid = pointIds.Get(index);
                auto point = coords.Get(pointid);
                auto dx = point[0] - queryPoint[0];
                auto dy = point[1] - queryPoint[1];
                auto dz = point[2] - queryPoint[2];

                auto distance2 = dx * dx + dy * dy + dz * dz;
                if (distance2 < min_distance)
                {
                  neareast = pointid;
                  min_distance = distance2;
                  nlayers = layer + 2;
                }
              }
            }
          }
        }
      }
    }

    nearestNeighborId = neareast;
    distance = vtkm::Sqrt(min_distance);
#endif
  }

private:
  vtkm::Vec<vtkm::FloatDefault, 3> Min;
  vtkm::Vec<vtkm::Id, 3> Dims;
  vtkm::Vec<vtkm::FloatDefault, 3> Dxdydz;

  CoordPortalType coords;

  IdPortalType pointIds;
  IdPortalType cellIds;
  IdPortalType cellLower;
  IdPortalType cellUpper;
};
}
}

#endif // vtk_m_exec_PointLocatorUniformGrid_h
