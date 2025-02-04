//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_exec_celllocatoruniformgrid_h
#define viskores_exec_celllocatoruniformgrid_h

#include <viskores/Bounds.h>
#include <viskores/Math.h>
#include <viskores/TopologyElementTag.h>
#include <viskores/Types.h>
#include <viskores/VecFromPortalPermute.h>

#include <viskores/cont/CellSetStructured.h>

#include <viskores/exec/CellInside.h>
#include <viskores/exec/ParametricCoordinates.h>

namespace viskores
{

namespace exec
{

class VISKORES_ALWAYS_EXPORT CellLocatorUniformGrid
{
public:
  VISKORES_CONT
  CellLocatorUniformGrid(const viskores::Id3 cellDims,
                         const viskores::Vec3f origin,
                         const viskores::Vec3f invSpacing,
                         const viskores::Vec3f maxPoint)
    : CellDims(cellDims)
    , MaxCellIds(viskores::Max(cellDims - viskores::Id3(1), viskores::Id3(0)))
    , Origin(origin)
    , InvSpacing(invSpacing)
    , MaxPoint(maxPoint)
  {
  }

  VISKORES_EXEC inline bool IsInside(const viskores::Vec3f& point) const
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

  VISKORES_EXEC
  viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                           viskores::Id& cellId,
                           viskores::Vec3f& parametric,
                           LastCell& viskoresNotUsed(lastCell)) const
  {
    return this->FindCell(point, cellId, parametric);
  }

  VISKORES_EXEC
  viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                           viskores::Id& cellId,
                           viskores::Vec3f& parametric) const
  {
    if (!this->IsInside(point))
    {
      cellId = -1;
      return viskores::ErrorCode::CellNotFound;
    }
    // Get the Cell Id from the point.
    viskores::Id3 logicalCell(0, 0, 0);

    viskores::Vec3f temp;
    temp = point - this->Origin;
    temp = temp * this->InvSpacing;

    //make sure that if we border the upper edge, we sample the correct cell
    logicalCell = viskores::Min(viskores::Id3(temp), this->MaxCellIds);

    cellId =
      (logicalCell[2] * this->CellDims[1] + logicalCell[1]) * this->CellDims[0] + logicalCell[0];
    parametric = temp - logicalCell;

    return viskores::ErrorCode::Success;
  }

private:
  viskores::Id3 CellDims;
  viskores::Id3 MaxCellIds;
  viskores::Vec3f Origin;
  viskores::Vec3f InvSpacing;
  viskores::Vec3f MaxPoint;
};
}
}

#endif //viskores_exec_celllocatoruniformgrid_h
