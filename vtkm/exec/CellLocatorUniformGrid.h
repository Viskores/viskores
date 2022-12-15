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
#include <vtkm/Math.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>
#include <vtkm/VecFromPortalPermute.h>

#include <vtkm/cont/CellSetStructured.h>

#include <vtkm/exec/CellInside.h>
#include <vtkm/exec/ParametricCoordinates.h>

namespace vtkm
{

namespace exec
{

class VTKM_ALWAYS_EXPORT CellLocatorUniformGrid
{
public:
  VTKM_CONT
  CellLocatorUniformGrid(const vtkm::Id3 cellDims,
                         const vtkm::Vec3f origin,
                         const vtkm::Vec3f invSpacing,
                         const vtkm::Vec3f maxPoint)
    : CellDims(cellDims)
    , MaxCellIds(vtkm::Max(cellDims - vtkm::Id3(1), vtkm::Id3(0)))
    , Origin(origin)
    , InvSpacing(invSpacing)
    , MaxPoint(maxPoint)
  {
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

  struct LastCell
  {
  };

  VTKM_EXEC
  vtkm::ErrorCode FindCell(const vtkm::Vec3f& point,
                           vtkm::Id& cellId,
                           vtkm::Vec3f& parametric,
                           LastCell& vtkmNotUsed(lastCell)) const
  {
    return this->FindCell(point, cellId, parametric);
  }

  VTKM_EXEC
  vtkm::ErrorCode FindCell(const vtkm::Vec3f& point,
                           vtkm::Id& cellId,
                           vtkm::Vec3f& parametric) const
  {
    if (!this->IsInside(point))
    {
      cellId = -1;
      return vtkm::ErrorCode::CellNotFound;
    }
    // Get the Cell Id from the point.
    vtkm::Id3 logicalCell(0, 0, 0);

    vtkm::Vec3f temp;
    temp = point - this->Origin;
    temp = temp * this->InvSpacing;

    //make sure that if we border the upper edge, we sample the correct cell
    logicalCell = vtkm::Min(vtkm::Id3(temp), this->MaxCellIds);

    cellId =
      (logicalCell[2] * this->CellDims[1] + logicalCell[1]) * this->CellDims[0] + logicalCell[0];
    parametric = temp - logicalCell;

    return vtkm::ErrorCode::Success;
  }

private:
  vtkm::Id3 CellDims;
  vtkm::Id3 MaxCellIds;
  vtkm::Vec3f Origin;
  vtkm::Vec3f InvSpacing;
  vtkm::Vec3f MaxPoint;
};
}
}

#endif //vtkm_exec_celllocatoruniformgrid_h
