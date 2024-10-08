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

/// @brief Structure for locating cells.
///
/// Use the `FindCell()` method to identify which cell contains a point in space.
/// The `FindCell()` method optionally takes a `LastCell` object, which is a
/// structure nested in this class. The `LastCell` object can help speed locating
/// cells for successive finds at nearby points.
///
/// This class is provided by `vtkm::cont::CellLocatorUniformGrid` when passed
/// to a worklet.
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

  /// @brief Structure capturing the location of a cell in the search structure.
  ///
  /// An object of this type is passed to and from the `FindCell()` method.
  /// If `FindCell()` is called successively with points near each other, the
  /// information in this object can reduce the time to find the cell.
  struct LastCell
  {
  };

  /// @brief Locate the cell containing the provided point.
  ///
  /// Given the point coordinate `point`, this method determines which cell
  /// contains that point. The identification of the cell is returned in
  /// the `cellId` reference parameter. The method also determines the
  /// cell's parametric coordinates to the point and returns that in the
  /// `parametric` reference parameter. This result can be used in functions
  /// like `vtkm::exec::CellInterpolate()`.
  ///
  /// `FindCell()` takes an optional `LastCell` parameter. This parameter
  /// captures the location of the found cell and can be passed to the next
  /// call of `FindCell()`. If the subsequent `FindCell()` call is for a
  /// point that is in or near the same cell, the operation may go faster.
  ///
  /// This method will return `vtkm::ErrorCode::Success` if a cell is found.
  /// If a cell is not found, `vtkm::ErrorCode::CellNotFound` is returned
  /// and `cellId` is set to `-1`.
  VTKM_EXEC vtkm::ErrorCode FindCell(const vtkm::Vec3f& point,
                                     vtkm::Id& cellId,
                                     vtkm::Vec3f& parametric,
                                     LastCell& lastCell) const
  {
    (void)lastCell;
    return this->FindCell(point, cellId, parametric);
  }

  /// @copydoc FindCell
  VTKM_EXEC vtkm::ErrorCode FindCell(const vtkm::Vec3f& point,
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
