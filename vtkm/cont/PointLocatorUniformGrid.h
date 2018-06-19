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
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_PointLocatorUniformGrid_h
#define vtk_m_cont_PointLocatorUniformGrid_h

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace cont
{
template <typename T>
class PointLocatorUniformGrid
{
public:
  PointLocatorUniformGrid(const vtkm::Vec<T, 3>& _min,
                          const vtkm::Vec<T, 3>& _max,
                          const vtkm::Vec<vtkm::Id, 3>& _dims)
    : Min(_min)
    , Max(_max)
    , Dims(_dims)
  {
  }

  class BinPointsWorklet : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn<> coord, FieldOut<> label);

    using ExecutionSignature = void(_1, _2);

    VTKM_CONT
    BinPointsWorklet(vtkm::Vec<T, 3> _min, vtkm::Vec<T, 3> _max, vtkm::Vec<vtkm::Id, 3> _dims)
      : Min(_min)
      , Dims(_dims)
      , Dxdydz((_max - Min) / Dims)
    {
    }

    template <typename CoordVecType, typename IdType>
    VTKM_EXEC void operator()(const CoordVecType& coord, IdType& label) const
    {
      vtkm::Vec<vtkm::Id, 3> ijk = (coord - Min) / Dxdydz;
      label = ijk[0] + ijk[1] * Dims[0] + ijk[2] * Dims[0] * Dims[1];
    }

  private:
    vtkm::Vec<T, 3> Min;
    vtkm::Vec<vtkm::Id, 3> Dims;
    vtkm::Vec<T, 3> Dxdydz;
  };

  template <typename DeviceAdapter>
  class Locator : public vtkm::exec::ExecutionObjectBase
  {
  public:
    using CoordPortalType = typename vtkm::cont::ArrayHandle<
      vtkm::Vec<T, 3>>::template ExecutionTypes<DeviceAdapter>::PortalConst;
    using IdPortalType = typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<
      DeviceAdapter>::PortalConst;

    VTKM_CONT
    Locator() = default;

    VTKM_CONT
    Locator(const vtkm::Vec<T, 3>& _min,
            const vtkm::Vec<T, 3>& _max,
            const vtkm::Vec<vtkm::Id, 3>& _dims,
            const CoordPortalType& coords,
            const IdPortalType& pointIds,
            const IdPortalType& cellLower,
            const IdPortalType& cellUpper)
      : Min(_min)
      , Dims(_dims)
      , Dxdydz((_max - Min) / Dims)
      , coords(coords)
      , pointIds(pointIds)
      , cellLower(cellLower)
      , cellUpper(cellUpper)
    {
    }

    /// \brief Nearest neighbor search using a Uniform Grid
    ///
    /// Parallel search of nearesat neighbor for each point in the \c queryPoints in the set of
    /// \c coords. Returns neareast neighbot in \c nearestNeighborIds and distances to nearest
    /// neighbor in \c distances.
    ///
    /// \param coords Point coordinates for training dataset.
    /// \param queryPoints Point coordinates to query for nearest neighbors.
    /// \param nearestNeighborIds Neareast neighbor in the training dataset for each points in
    ///                            the test set
    /// \param distances Distance between query points and their nearest neighbors.
    /// \param device Tag for selecting device adapter.
    VTKM_EXEC
    void FindNearestPoint(const vtkm::Vec<T, 3>& queryPoint,
                          vtkm::Id& nearestNeighborId,
                          T& distance) const
    {
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
    };

  private:
    vtkm::Vec<T, 3> Min;
    vtkm::Vec<vtkm::Id, 3> Dims;
    vtkm::Vec<T, 3> Dxdydz;

    CoordPortalType coords;

    IdPortalType pointIds;
    IdPortalType cellIds;
    IdPortalType cellLower;
    IdPortalType cellUpper;
  };

  /// \brief Construct a 3D uniform grid for nearest neighbor search.
  ///
  /// \param coords An ArrayHandle of x, y, z coordinates of input points.
  /// \param device Tag for selecting device adapter
  template <typename DeviceAdapter>
  void Build(const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
             DeviceAdapter vtkmNotUsed(device))
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    // Save training data points.
    Algorithm::Copy(coords, Coords);

    // generate unique id for each input point
    vtkm::cont::ArrayHandleCounting<vtkm::Id> pointCounting(0, 1, coords.GetNumberOfValues());
    Algorithm::Copy(pointCounting, PointIds);

    // bin points into cells and give each of them the cell id.
    BinPointsWorklet cellIdWorklet(Min, Max, Dims);
    vtkm::worklet::DispatcherMapField<BinPointsWorklet, DeviceAdapter> dispatchCellId(
      cellIdWorklet);
    dispatchCellId.Invoke(coords, CellIds);

    // Group points of the same cell together by sorting them according to the cell ids
    Algorithm::SortByKey(CellIds, PointIds);

    // for each cell, find the lower and upper bound of indices to the sorted point ids.
    vtkm::cont::ArrayHandleCounting<vtkm::Id> cell_ids_counting(0, 1, Dims[0] * Dims[1] * Dims[2]);
    Algorithm::UpperBounds(CellIds, cell_ids_counting, CellUpper);
    Algorithm::LowerBounds(CellIds, cell_ids_counting, CellLower);
  }

  template <typename DeviceAdapter>
  Locator<DeviceAdapter> PrepareForExecution(DeviceAdapter)
  {
    // TODO: lifetime of coords???
    return Locator<DeviceAdapter>(Min,
                                  Max,
                                  Dims,
                                  Coords.PrepareForInput(DeviceAdapter()),
                                  PointIds.PrepareForInput(DeviceAdapter()),
                                  CellLower.PrepareForInput(DeviceAdapter()),
                                  CellUpper.PrepareForInput(DeviceAdapter()));
  }

private:
  vtkm::Vec<T, 3> Min;
  vtkm::Vec<T, 3> Max;
  vtkm::Vec<vtkm::Id, 3> Dims;

  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> Coords;
  vtkm::cont::ArrayHandle<vtkm::Id> PointIds;
  vtkm::cont::ArrayHandle<vtkm::Id> CellIds;
  vtkm::cont::ArrayHandle<vtkm::Id> CellLower;
  vtkm::cont::ArrayHandle<vtkm::Id> CellUpper;
};
}
}
#endif //vtk_m_cont_PointLocatorUniformGrid_h
