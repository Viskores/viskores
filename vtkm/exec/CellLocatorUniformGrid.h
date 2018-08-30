//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_exec_celllocatoruniformgrid_h
#define vtkm_exec_celllocatoruniformgrid_h

#include <vtkm/Bounds.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>
#include <vtkm/VecFromPortalPermute.h>

#include <vtkm/cont/CellSetStructured.h>

#include <vtkm/exec/CellInside.h>
#include <vtkm/exec/CellLocator.h>
#include <vtkm/exec/ParametricCoordinates.h>

namespace vtkm
{

namespace exec
{

template <typename DeviceAdapter>
class CellLocatorUniformGrid : public vtkm::exec::CellLocator
{
public:
  VTKM_CONT
  CellLocatorUniformGrid(const vtkm::Bounds& bounds,
                         const vtkm::Vec<vtkm::Id, 3>& dims,
                         const vtkm::Vec<vtkm::FloatDefault, 3> rangeTransform,
                         const vtkm::Id planeSize,
                         const vtkm::Id rowSize,
                         const vtkm::cont::CellSetStructured<3>& cellSet,
                         const vtkm::cont::ArrayHandleVirtualCoordinates& coords,
                         DeviceAdapter)
    : Bounds(bounds)
    , Dims(dims)
    , RangeTransform(rangeTransform)
    , PlaneSize(planeSize)
    , RowSize(rowSize)
  {
    CellSet = cellSet.PrepareForInput(DeviceAdapter(), FromType(), ToType());
    Coords = coords.PrepareForInput(DeviceAdapter());
  }

  VTKM_EXEC
  void FindCell(const vtkm::Vec<vtkm::FloatDefault, 3>& point,
                vtkm::Id& cellId,
                vtkm::Vec<vtkm::FloatDefault, 3>& parametric,
                const vtkm::exec::FunctorBase& worklet) const override
  {
    if (!Bounds.Contains(point))
    {
      cellId = -1;
      return;
    }
    // Get the Cell Id from the point.
    vtkm::Vec<vtkm::Id, 3> logicalCell;
    logicalCell[0] =
      static_cast<vtkm::Id>(vtkm::Floor((point[0] - Bounds.X.Min) * RangeTransform[0]));
    logicalCell[1] =
      static_cast<vtkm::Id>(vtkm::Floor((point[1] - Bounds.Y.Min) * RangeTransform[1]));
    logicalCell[2] =
      static_cast<vtkm::Id>(vtkm::Floor((point[2] - Bounds.Z.Min) * RangeTransform[2]));
    // Get the actual cellId, from the logical cell index of the cell
    cellId = logicalCell[2] * PlaneSize + logicalCell[1] * RowSize + logicalCell[0];

    bool success = false;
    using IndicesType = typename CellSetPortal::IndicesType;
    IndicesType cellPointIndices = CellSet.GetIndices(cellId);
    vtkm::VecFromPortalPermute<IndicesType, CoordsPortal> cellPoints(&cellPointIndices, Coords);
    auto cellShape = CellSet.GetCellShape(cellId);
    // Get Parametric Coordinates from the cell, for the point.
    parametric = vtkm::exec::WorldCoordinatesToParametricCoordinates(
      cellPoints, point, cellShape, success, worklet);
  }

private:
  using FromType = vtkm::TopologyElementTagPoint;
  using ToType = vtkm::TopologyElementTagCell;
  using CellSetPortal = typename vtkm::cont::CellSetStructured<
    3>::template ExecutionTypes<DeviceAdapter, FromType, ToType>::ExecObjectType;
  using CoordsPortal = typename vtkm::cont::ArrayHandleVirtualCoordinates::template ExecutionTypes<
    DeviceAdapter>::PortalConst;

  vtkm::Bounds Bounds;
  vtkm::Vec<vtkm::Id, 3> Dims;
  vtkm::Vec<vtkm::FloatDefault, 3> RangeTransform;
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;
  CellSetPortal CellSet;
  CoordsPortal Coords;
};
}
}

#endif //vtkm_exec_celllocatoruniformgrid_h
