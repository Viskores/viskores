//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_exec_celllocatorrectilineargrid_h
#define vtkm_exec_celllocatorrectilineargrid_h

#include <vtkm/Bounds.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>
#include <vtkm/VecFromPortalPermute.h>

#include <vtkm/cont/CellSetStructured.h>

#include <vtkm/exec/CellInside.h>
#include <vtkm/exec/CellLocator.h>
#include <vtkm/exec/ConnectivityStructured.h>
#include <vtkm/exec/ParametricCoordinates.h>

namespace vtkm
{

namespace exec
{

template <typename DeviceAdapter>
class VTKM_ALWAYS_EXPORT CellLocatorRectilinearGrid : public vtkm::exec::CellLocator
{
private:
  using FromType = vtkm::TopologyElementTagPoint;
  using ToType = vtkm::TopologyElementTagCell;
  using CellSetPortal = vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                                           vtkm::TopologyElementTagCell,
                                                           3>;
  using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using RectilinearType =
    vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
  using AxisPortalType = typename AxisHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using RectilinearPortalType =
    typename RectilinearType::template ExecutionTypes<DeviceAdapter>::PortalConst;

public:
  VTKM_CONT
  CellLocatorRectilinearGrid(const vtkm::Id planeSize,
                             const vtkm::Id rowSize,
                             const vtkm::cont::CellSetStructured<3>& cellSet,
                             const RectilinearType& coords,
                             DeviceAdapter)
    : PlaneSize(planeSize)
    , RowSize(rowSize)
    , CellSet(cellSet.PrepareForInput(DeviceAdapter(), FromType(), ToType()))
    , Coords(coords.PrepareForInput(DeviceAdapter()))
    , PointDimensions(cellSet.GetPointDimensions())
  {
    this->AxisPortals[0] = Coords.GetFirstPortal();
    this->AxisPortals[1] = Coords.GetSecondPortal();
    this->AxisPortals[2] = Coords.GetThirdPortal();

    this->MinPoint[0] = coords.GetPortalConstControl().GetFirstPortal().Get(0);
    this->MinPoint[1] = coords.GetPortalConstControl().GetSecondPortal().Get(0);
    this->MinPoint[2] = coords.GetPortalConstControl().GetThirdPortal().Get(0);

    this->MaxPoint[0] = coords.GetPortalConstControl().GetFirstPortal().Get(PointDimensions[0] - 1);
    this->MaxPoint[1] =
      coords.GetPortalConstControl().GetSecondPortal().Get(PointDimensions[1] - 1);
    this->MaxPoint[2] = coords.GetPortalConstControl().GetThirdPortal().Get(PointDimensions[2] - 1);
  }

  VTKM_EXEC_CONT virtual ~CellLocatorRectilinearGrid() noexcept
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC
  inline bool IsInside(const vtkm::Vec<vtkm::FloatDefault, 3>& point) const
  {
    bool inside = true;
    if (point[0] < this->MinPoint[0] || point[0] > this->MaxPoint[0])
      inside = false;
    if (point[1] < this->MinPoint[1] || point[1] > this->MaxPoint[1])
      inside = false;
    if (point[2] < this->MinPoint[2] || point[2] > this->MaxPoint[2])
      inside = false;
    return inside;
  }

  VTKM_EXEC
  void FindCell(const vtkm::Vec<vtkm::FloatDefault, 3>& point,
                vtkm::Id& cellId,
                vtkm::Vec<vtkm::FloatDefault, 3>& parametric,
                const vtkm::exec::FunctorBase& worklet) const override
  {
    if (!IsInside(point))
    {
      cellId = -1;
      return;
    }

    // Get the Cell Id from the point.
    vtkm::Vec<vtkm::Id, 3> logicalCell(0, 0, 0);
    for (vtkm::Int32 dim = 0; dim < 3; ++dim)
    {
      //
      // When searching for points, we consider the max value of the cell
      // to be apart of the next cell. If the point falls on the boundary of the
      // data set, then it is technically inside a cell. This checks for that case
      //
      if (point[dim] == MaxPoint[dim])
      {
        logicalCell[dim] = PointDimensions[dim] - 2;
        continue;
      }

      bool found = false;
      vtkm::FloatDefault minVal = this->AxisPortals[dim].Get(logicalCell[dim]);
      const vtkm::Id searchDir = (point[dim] - minVal >= 0.f) ? 1 : -1;
      vtkm::FloatDefault maxVal = this->AxisPortals[dim].Get(logicalCell[dim] + 1);

      while (!found)
      {
        if (point[dim] >= minVal && point[dim] < maxVal)
        {
          found = true;
          continue;
        }

        logicalCell[dim] += searchDir;
        vtkm::Id nextCellId = searchDir == 1 ? logicalCell[dim] + 1 : logicalCell[dim];
        vtkm::FloatDefault next = this->AxisPortals[dim].Get(nextCellId);
        if (searchDir == 1)
        {
          minVal = maxVal;
          maxVal = next;
        }
        else
        {
          maxVal = minVal;
          minVal = next;
        }
      }
    }

    // Get the actual cellId, from the logical cell index of the cell
    cellId = logicalCell[2] * this->PlaneSize + logicalCell[1] * this->RowSize + logicalCell[0];

    bool success = false;
    using IndicesType = typename CellSetPortal::IndicesType;
    IndicesType cellPointIndices = this->CellSet.GetIndices(cellId);
    vtkm::VecFromPortalPermute<IndicesType, RectilinearPortalType> cellPoints(&cellPointIndices,
                                                                              Coords);
    auto cellShape = this->CellSet.GetCellShape(cellId);
    // Get Parametric Coordinates from the cell, for the point.
    parametric = vtkm::exec::WorldCoordinatesToParametricCoordinates(
      cellPoints, point, cellShape, success, worklet);
  }

private:
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;

  CellSetPortal CellSet;
  RectilinearPortalType Coords;
  AxisPortalType AxisPortals[3];
  vtkm::Id3 PointDimensions;
  vtkm::Vec<vtkm::FloatDefault, 3> MinPoint;
  vtkm::Vec<vtkm::FloatDefault, 3> MaxPoint;
};
} //namespace exec
} //namespace vtkm

#endif //vtkm_exec_celllocatorrectilineargrid_h
