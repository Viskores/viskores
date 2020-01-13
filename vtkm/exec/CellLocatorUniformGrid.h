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

template <typename DeviceAdapter, vtkm::IdComponent dimensions>
class VTKM_ALWAYS_EXPORT CellLocatorUniformGrid final : public vtkm::exec::CellLocator
{
private:
  using VisitType = vtkm::TopologyElementTagCell;
  using IncidentType = vtkm::TopologyElementTagPoint;
  using CellSetPortal = vtkm::exec::ConnectivityStructured<VisitType, IncidentType, dimensions>;
  using CoordsPortal = typename vtkm::cont::ArrayHandleVirtualCoordinates::template ExecutionTypes<
    DeviceAdapter>::PortalConst;

public:
  VTKM_CONT
  CellLocatorUniformGrid(const vtkm::Id3 cellDims,
                         const vtkm::Id3 pointDims,
                         const vtkm::Vec3f origin,
                         const vtkm::Vec3f invSpacing,
                         const vtkm::Vec3f maxPoint,
                         const vtkm::cont::ArrayHandleVirtualCoordinates& coords,
                         DeviceAdapter)
    : CellDims(cellDims)
    , PointDims(pointDims)
    , Origin(origin)
    , InvSpacing(invSpacing)
    , MaxPoint(maxPoint)
    , Coords(coords.PrepareForInput(DeviceAdapter()))
  {
  }

  VTKM_EXEC_CONT virtual ~CellLocatorUniformGrid() noexcept override
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC inline bool IsInside(const vtkm::Vec3f& point) const
  {
    bool inside = true;
    if (point[0] < this->Origin[0] || point[0] > this->MaxPoint[0])
      inside = false;
    if (point[1] < this->Origin[1] || point[1] > this->MaxPoint[1])
      inside = false;
    if (point[2] < this->Origin[2] || point[2] > this->MaxPoint[2])
      inside = false;
    return inside;
  }

  VTKM_EXEC
  void FindCell(const vtkm::Vec3f& point,
                vtkm::Id& cellId,
                vtkm::Vec3f& parametric,
                const vtkm::exec::FunctorBase& worklet) const override
  {
    (void)worklet; //suppress unused warning
    if (!this->IsInside(point))
    {
      cellId = -1;
      return;
    }
    // Get the Cell Id from the point.
    vtkm::Id3 logicalCell(0, 0, 0);

    vtkm::Vec3f temp;
    temp = point - this->Origin;
    temp = temp * this->InvSpacing;

    //make sure that if we border the upper edge, we sample the correct cell
    logicalCell = temp;
    if (logicalCell[0] == this->CellDims[0])
    {
      logicalCell[0]--;
    }
    if (logicalCell[1] == this->CellDims[1])
    {
      logicalCell[1]--;
    }
    if (logicalCell[2] == this->CellDims[2])
    {
      logicalCell[2]--;
    }
    if (dimensions == 2)
      logicalCell[2] = 0;
    cellId =
      (logicalCell[2] * this->CellDims[1] + logicalCell[1]) * this->CellDims[0] + logicalCell[0];
    parametric = temp - logicalCell;
  }

private:
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
  vtkm::Vec3f Origin;
  vtkm::Vec3f InvSpacing;
  vtkm::Vec3f MaxPoint;
  CoordsPortal Coords;
};
}
}

#endif //vtkm_exec_celllocatoruniformgrid_h
