//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
class VTKM_ALWAYS_EXPORT CellLocatorUniformGrid : public vtkm::exec::CellLocator
{
private:
  using FromType = vtkm::TopologyElementTagPoint;
  using ToType = vtkm::TopologyElementTagCell;
  using CellSetPortal = vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                                           vtkm::TopologyElementTagCell,
                                                           3>;
  using CoordsPortal = typename vtkm::cont::ArrayHandleVirtualCoordinates::template ExecutionTypes<
    DeviceAdapter>::PortalConst;

public:
  VTKM_CONT
  CellLocatorUniformGrid(const vtkm::Bounds& bounds,
                         const vtkm::Vec<vtkm::FloatDefault, 3> rangeTransform,
                         const vtkm::Vec<vtkm::Id, 3> cellDims,
                         const vtkm::cont::CellSetStructured<3>& cellSet,
                         const vtkm::cont::ArrayHandleVirtualCoordinates& coords,
                         DeviceAdapter)
    : Bounds(bounds)
    , RangeTransform(rangeTransform)
    , CellDims(cellDims)
    , PlaneSize(cellDims[0] * cellDims[1])
    , RowSize(cellDims[0])
    , CellSet(cellSet.PrepareForInput(DeviceAdapter(), FromType(), ToType()))
    , Coords(coords.PrepareForInput(DeviceAdapter()))
  {
  }

  VTKM_EXEC_CONT virtual ~CellLocatorUniformGrid() noexcept
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
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
    logicalCell[0] = (point[0] == Bounds.X.Max)
      ? CellDims[0] - 1
      : static_cast<vtkm::Id>(vtkm::Floor((point[0] - Bounds.X.Min) * RangeTransform[0]));
    logicalCell[1] = (point[1] == Bounds.Y.Max)
      ? CellDims[1] - 1
      : static_cast<vtkm::Id>(vtkm::Floor((point[1] - Bounds.Y.Min) * RangeTransform[1]));
    logicalCell[2] = (point[2] == Bounds.Z.Max)
      ? CellDims[2] - 1
      : static_cast<vtkm::Id>(vtkm::Floor((point[2] - Bounds.Z.Min) * RangeTransform[2]));

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
  vtkm::Bounds Bounds;
  vtkm::Vec<vtkm::FloatDefault, 3> RangeTransform;
  vtkm::Vec<vtkm::Id, 3> CellDims;
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;
  CellSetPortal CellSet;
  CoordsPortal Coords;
};
}
}

#endif //vtkm_exec_celllocatoruniformgrid_h
