//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_PointLocatorUniformGrid_h
#define vtk_m_exec_PointLocatorUniformGrid_h

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/exec/PointLocator.h>

#include <vtkm/VectorAnalysis.h>

namespace vtkm
{
namespace exec
{

template <typename DeviceAdapter>
class PointLocatorUniformGrid : public vtkm::exec::PointLocator
{
public:
  using CoordPortalType =
    typename vtkm::cont::ArrayHandleVirtualCoordinates::template ExecutionTypes<
      DeviceAdapter>::PortalConst;
  using IdPortalType =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapter>::PortalConst;


  PointLocatorUniformGrid() = default;

  PointLocatorUniformGrid(const vtkm::Vec<vtkm::FloatDefault, 3>& min,
                          const vtkm::Vec<vtkm::FloatDefault, 3>& max,
                          const vtkm::Vec<vtkm::Id, 3>& dims,
                          const CoordPortalType& coords,
                          const IdPortalType& pointIds,
                          const IdPortalType& cellLower,
                          const IdPortalType& cellUpper)
    : Min(min)
    , Dims(dims)
    , Dxdydz((max - Min) / Dims)
    , Coords(coords)
    , PointIds(pointIds)
    , CellLower(cellLower)
    , CellUpper(cellUpper)
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
  /// \param distance2 Squared distance between query points and their nearest neighbors.
  VTKM_EXEC virtual void FindNearestNeighbor(const vtkm::Vec<vtkm::FloatDefault, 3>& queryPoint,
                                             vtkm::Id& nearestNeighborId,
                                             vtkm::FloatDefault& distance2) const override
  {
    //std::cout << "FindNeareastNeighbor: " << queryPoint << std::endl;
    vtkm::Id3 ijk = (queryPoint - this->Min) / this->Dxdydz;
    ijk = vtkm::Max(ijk, vtkm::Id3(0));
    ijk = vtkm::Min(ijk, this->Dims - vtkm::Id3(1));

    nearestNeighborId = -1;
    distance2 = vtkm::Infinity<vtkm::FloatDefault>();

    this->FindInCell(queryPoint, ijk, nearestNeighborId, distance2);

    // TODO: This might stop looking before the absolute nearest neighbor is found.
    vtkm::Id maxLevel = vtkm::Max(vtkm::Max(this->Dims[0], this->Dims[1]), this->Dims[2]);
    for (vtkm::Id level = 1; (nearestNeighborId < 0) && (level < maxLevel); ++level)
    {
      this->FindInBox(queryPoint, ijk, level, nearestNeighborId, distance2);
    }
  }

private:
  vtkm::Vec<vtkm::FloatDefault, 3> Min;
  vtkm::Vec<vtkm::Id, 3> Dims;
  vtkm::Vec<vtkm::FloatDefault, 3> Dxdydz;

  CoordPortalType Coords;

  IdPortalType PointIds;
  IdPortalType CellLower;
  IdPortalType CellUpper;

  VTKM_EXEC void FindInCell(const vtkm::Vec<vtkm::FloatDefault, 3>& queryPoint,
                            const vtkm::Id3& ijk,
                            vtkm::Id& nearestNeighborId,
                            vtkm::FloatDefault& nearestDistance2) const
  {
    vtkm::Id cellId = ijk[0] + (ijk[1] * this->Dims[0]) + (ijk[2] * this->Dims[0] * this->Dims[1]);
    vtkm::Id lower = this->CellLower.Get(cellId);
    vtkm::Id upper = this->CellUpper.Get(cellId);
    for (vtkm::Id index = lower; index < upper; index++)
    {
      vtkm::Id pointid = this->PointIds.Get(index);
      vtkm::Vec<vtkm::FloatDefault, 3> point = this->Coords.Get(pointid);
      vtkm::FloatDefault distance2 = vtkm::MagnitudeSquared(point - queryPoint);
      if (distance2 < nearestDistance2)
      {
        nearestNeighborId = pointid;
        nearestDistance2 = distance2;
      }
    }
  }

  VTKM_EXEC void FindInBox(const vtkm::Vec<vtkm::FloatDefault, 3>& queryPoint,
                           const vtkm::Id3& boxCenter,
                           vtkm::Id level,
                           vtkm::Id& nearestNeighborId,
                           vtkm::FloatDefault& nearestDistance2) const
  {
    if ((boxCenter[0] - level) >= 0)
    {
      this->FindInXPlane(
        queryPoint, boxCenter - vtkm::Id3(level, 0, 0), level, nearestNeighborId, nearestDistance2);
    }
    if ((boxCenter[0] + level) < this->Dims[0])
    {
      this->FindInXPlane(
        queryPoint, boxCenter + vtkm::Id3(level, 0, 0), level, nearestNeighborId, nearestDistance2);
    }

    if ((boxCenter[1] - level) >= 0)
    {
      this->FindInYPlane(
        queryPoint, boxCenter - vtkm::Id3(0, level, 0), level, nearestNeighborId, nearestDistance2);
    }
    if ((boxCenter[1] + level) < this->Dims[1])
    {
      this->FindInYPlane(
        queryPoint, boxCenter - vtkm::Id3(0, level, 0), level, nearestNeighborId, nearestDistance2);
    }

    if ((boxCenter[2] - level) >= 0)
    {
      this->FindInZPlane(
        queryPoint, boxCenter - vtkm::Id3(0, 0, level), level, nearestNeighborId, nearestDistance2);
    }
    if ((boxCenter[2] + level) < this->Dims[2])
    {
      this->FindInZPlane(
        queryPoint, boxCenter - vtkm::Id3(0, 0, level), level, nearestNeighborId, nearestDistance2);
    }
  }

  VTKM_EXEC void FindInPlane(const vtkm::Vec<vtkm::FloatDefault, 3>& queryPoint,
                             const vtkm::Id3& planeCenter,
                             const vtkm::Id3& div,
                             const vtkm::Id3& mod,
                             const vtkm::Id3& origin,
                             vtkm::Id numInPlane,
                             vtkm::Id& nearestNeighborId,
                             vtkm::FloatDefault& nearestDistance2) const
  {
    for (vtkm::Id index = 0; index < numInPlane; ++index)
    {
      vtkm::Id3 ijk = planeCenter + vtkm::Id3(index) / div +
        vtkm::Id3(index % mod[0], index % mod[1], index % mod[2]) + origin;
      if ((ijk[0] >= 0) && (ijk[0] < this->Dims[0]) && (ijk[1] >= 0) && (ijk[1] < this->Dims[1]) &&
          (ijk[2] >= 0) && (ijk[2] < this->Dims[2]))
      {
        this->FindInCell(queryPoint, ijk, nearestNeighborId, nearestDistance2);
      }
    }
  }

  VTKM_EXEC void FindInXPlane(const vtkm::Vec<vtkm::FloatDefault, 3>& queryPoint,
                              const vtkm::Id3& planeCenter,
                              vtkm::Id level,
                              vtkm::Id& nearestNeighborId,
                              vtkm::FloatDefault& nearestDistance2) const
  {
    vtkm::Id yWidth = (2 * level) + 1;
    vtkm::Id zWidth = (2 * level) + 1;
    vtkm::Id3 div = { yWidth * zWidth, yWidth * zWidth, yWidth };
    vtkm::Id3 mod = { 1, yWidth, 1 };
    vtkm::Id3 origin = { 0, -level, -level };
    vtkm::Id numInPlane = yWidth * zWidth;
    this->FindInPlane(
      queryPoint, planeCenter, div, mod, origin, numInPlane, nearestNeighborId, nearestDistance2);
  }

  VTKM_EXEC void FindInYPlane(const vtkm::Vec<vtkm::FloatDefault, 3>& queryPoint,
                              vtkm::Id3 planeCenter,
                              vtkm::Id level,
                              vtkm::Id& nearestNeighborId,
                              vtkm::FloatDefault& nearestDistance2) const
  {
    vtkm::Id xWidth = (2 * level) - 1;
    vtkm::Id zWidth = (2 * level) + 1;
    vtkm::Id3 div = { xWidth * zWidth, xWidth * zWidth, xWidth };
    vtkm::Id3 mod = { xWidth, 1, 1 };
    vtkm::Id3 origin = { -level + 1, 0, -level };
    vtkm::Id numInPlane = xWidth * zWidth;
    this->FindInPlane(
      queryPoint, planeCenter, div, mod, origin, numInPlane, nearestNeighborId, nearestDistance2);
  }

  VTKM_EXEC void FindInZPlane(const vtkm::Vec<vtkm::FloatDefault, 3>& queryPoint,
                              vtkm::Id3 planeCenter,
                              vtkm::Id level,
                              vtkm::Id& nearestNeighborId,
                              vtkm::FloatDefault& nearestDistance2) const
  {
    vtkm::Id xWidth = (2 * level) - 1;
    vtkm::Id yWidth = (2 * level) - 1;
    vtkm::Id3 div = { xWidth * yWidth, xWidth, xWidth * yWidth };
    vtkm::Id3 mod = { xWidth, 1, 1 };
    vtkm::Id3 origin = { -level + 1, -level + 1, 0 };
    vtkm::Id numInPlane = xWidth * yWidth;
    this->FindInPlane(
      queryPoint, planeCenter, div, mod, origin, numInPlane, nearestNeighborId, nearestDistance2);
  }
};
}
}

#endif // vtk_m_exec_PointLocatorUniformGrid_h
