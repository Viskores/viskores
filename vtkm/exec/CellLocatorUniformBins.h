//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_CellLocatorUniformBins_h
#define vtk_m_exec_CellLocatorUniformBins_h

#include <vtkm/exec/CellInside.h>
#include <vtkm/exec/ParametricCoordinates.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>

#include <vtkm/TopologyElementTag.h>
#include <vtkm/VecFromPortalPermute.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace exec
{

//--------------------------------------------------------------------
template <typename CellStructureType>
class VTKM_ALWAYS_EXPORT CellLocatorUniformBins
{
  template <typename T>
  using ReadPortal = typename vtkm::cont::ArrayHandle<T>::ReadPortalType;

  using CoordsPortalType =
    typename vtkm::cont::CoordinateSystem::MultiplexerArrayType::ReadPortalType;

  using CellIdArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using CellIdOffsetArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using CellIdReadPortal =
    typename vtkm::cont::ArrayHandleGroupVecVariable<CellIdArrayType,
                                                     CellIdOffsetArrayType>::ReadPortalType;

public:
  template <typename CellSetType>
  VTKM_CONT CellLocatorUniformBins(
    const vtkm::Id3& cellDims,
    const vtkm::Vec3f& origin,
    const vtkm::Vec3f& maxPoint,
    const vtkm::Vec3f& invSpacing,
    const vtkm::Id3& maxCellIds,
    const vtkm::cont::ArrayHandleGroupVecVariable<CellIdArrayType, CellIdOffsetArrayType>& cellIds,
    const CellSetType& cellSet,
    const vtkm::cont::CoordinateSystem& coords,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
    : CellDims(cellDims)
    , Origin(origin)
    , MaxPoint(maxPoint)
    , InvSpacing(invSpacing)
    , MaxCellIds(maxCellIds)
    , CellIds(cellIds.PrepareForInput(device, token))
    , CellSet(cellSet.PrepareForInput(device,
                                      vtkm::TopologyElementTagCell{},
                                      vtkm::TopologyElementTagPoint{},
                                      token))
    , Coords(coords.GetDataAsMultiplexer().PrepareForInput(device, token))
  {
  }

  struct LastCell
  {
    vtkm::Id CellId = -1;
    vtkm::Id BinIdx = -1;
  };

  VTKM_EXEC
  vtkm::ErrorCode FindCell(const vtkm::Vec3f& point,
                           vtkm::Id& cellId,
                           vtkm::Vec3f& parametric) const
  {
    LastCell lastCell;
    return this->FindCellImpl(point, cellId, parametric, lastCell);
  }

  VTKM_EXEC
  vtkm::ErrorCode FindCell(const vtkm::Vec3f& point,
                           vtkm::Id& cellId,
                           vtkm::Vec3f& parametric,
                           LastCell& lastCell) const
  {
    vtkm::Id binIdx = this->FindBinIdx(point);

    if (binIdx == -1)
    {
      lastCell.CellId = -1;
      lastCell.BinIdx = -1;
      cellId = -1;
      return vtkm::ErrorCode::CellNotFound;
    }
    //See if the point is still in the same bin.
    else if (binIdx == lastCell.BinIdx && this->LastCellValid(lastCell))
    {
      vtkm::Vec3f pc;
      //Check the last cell first.
      if (this->PointInCell(point, lastCell.CellId, pc))
      {
        parametric = pc;
        cellId = lastCell.CellId;
        return vtkm::ErrorCode::Success;
      }
      //Otherwise, check cells in the bin, but skip lastCell.CellId
      else if (this->PointInBin(point, lastCell.BinIdx, cellId, pc, lastCell.CellId))
      {
        parametric = pc;
        return vtkm::ErrorCode::Success;
      }
    }
    //if cell still not found, drop to the general find cell below.

    //LastCell not initialized, or not in the same bin: do a full test.
    //Since already computed the binIdx, re-use it.
    return this->FindCellImpl(point, cellId, parametric, lastCell, binIdx);
  }

  VTKM_DEPRECATED(1.6, "Locators are no longer pointers. Use . operator.")
  VTKM_EXEC CellLocatorUniformBins* operator->() { return this; }
  VTKM_DEPRECATED(1.6, "Locators are no longer pointers. Use . operator.")
  VTKM_EXEC const CellLocatorUniformBins* operator->() const { return this; }

private:
  VTKM_EXEC vtkm::Id FindBinIdx(const vtkm::Vec3f& point) const
  {
    if (!this->IsInside(point))
      return -1;

    vtkm::Vec3f temp;
    temp = point - this->Origin;
    temp = temp * this->InvSpacing;

    //make sure that if we border the upper edge, we sample the correct cell
    vtkm::Id3 logicalCell = vtkm::Min(vtkm::Id3(temp), this->MaxCellIds);

    vtkm::Id binIdx =
      (logicalCell[2] * this->CellDims[1] + logicalCell[1]) * this->CellDims[0] + logicalCell[0];

    return binIdx;
  }

  VTKM_EXEC bool LastCellValid(const LastCell& lastCell) const
  {
    return lastCell.BinIdx >= 0 && lastCell.BinIdx < this->CellIds.GetNumberOfValues() &&
      lastCell.CellId >= 0 && lastCell.CellId < this->CellSet.GetNumberOfElements();
  }

  VTKM_EXEC bool IsInside(const vtkm::Vec3f& point) const
  {
    if (point[0] < this->Origin[0] || point[0] > this->MaxPoint[0])
      return false;
    if (point[1] < this->Origin[1] || point[1] > this->MaxPoint[1])
      return false;
    if (point[2] < this->Origin[2] || point[2] > this->MaxPoint[2])
      return false;

    return true;
  }

  VTKM_EXEC
  vtkm::ErrorCode FindCellImpl(const vtkm::Vec3f& point,
                               vtkm::Id& cellId,
                               vtkm::Vec3f& parametric,
                               LastCell& lastCell,
                               vtkm::Id ptBinIdx = -1) const
  {
    lastCell.CellId = -1;
    lastCell.BinIdx = -1;

    //if ptBinIdx is set, use it. Otherwise, compute the bin idx.
    vtkm::Id binIdx = -1;
    if (ptBinIdx == -1)
      binIdx = this->FindBinIdx(point);
    else
      binIdx = ptBinIdx;

    //point not in a bin. return not found.
    if (binIdx == -1)
    {
      cellId = -1;
      return vtkm::ErrorCode::CellNotFound;
    }

    //search cells in the bin.
    vtkm::Vec3f pc;
    if (this->PointInBin(point, binIdx, cellId, pc))
    {
      parametric = pc;
      lastCell.CellId = cellId;
      lastCell.BinIdx = binIdx;
      return vtkm::ErrorCode::Success;
    }

    return vtkm::ErrorCode::CellNotFound;
  }

  template <typename PointsVecType>
  VTKM_EXEC vtkm::Bounds ComputeCellBounds(const PointsVecType& points) const
  {
    auto numPoints = vtkm::VecTraits<PointsVecType>::GetNumberOfComponents(points);

    vtkm::Bounds bounds;
    for (vtkm::IdComponent i = 0; i < numPoints; ++i)
      bounds.Include(points[i]);

    return bounds;
  }

  // TODO: This function may return false positives for non 3D cells as the
  // tests are done on the projection of the point on the cell. Extra checks
  // should be added to test if the point actually falls on the cell.
  template <typename CellShapeTag, typename CoordsType>
  VTKM_EXEC vtkm::ErrorCode PointInsideCell(vtkm::Vec3f point,
                                            CellShapeTag cellShape,
                                            CoordsType cellPoints,
                                            vtkm::Vec3f& parametricCoordinates,
                                            bool& inside) const
  {
    auto bounds = this->ComputeCellBounds(cellPoints);
    if (bounds.Contains(point))
    {
      VTKM_RETURN_ON_ERROR(vtkm::exec::WorldCoordinatesToParametricCoordinates(
        cellPoints, point, cellShape, parametricCoordinates));
      inside = vtkm::exec::CellInside(parametricCoordinates, cellShape);
    }
    else
    {
      inside = false;
    }
    // Return success error code even point is not inside this cell
    return vtkm::ErrorCode::Success;
  }

  VTKM_EXEC
  bool PointInBin(const vtkm::Vec3f& point,
                  const vtkm::Id& binIdx,
                  vtkm::Id& cellId,
                  vtkm::Vec3f& parametric,
                  const vtkm::Id& skipCellId = -1) const
  {
    auto binIds = this->CellIds.Get(binIdx);

    vtkm::Vec3f pc;
    for (vtkm::IdComponent i = 0; i < binIds.GetNumberOfComponents(); i++)
    {
      vtkm::Id cid = binIds[i];
      if (cid != skipCellId && this->PointInCell(point, cid, pc))
      {
        cellId = cid;
        parametric = pc;
        return true;
      }
    }

    return false;
  }

  VTKM_EXEC
  bool PointInCell(const vtkm::Vec3f& point, const vtkm::Id& cid, vtkm::Vec3f& parametric) const
  {
    auto indices = this->CellSet.GetIndices(cid);
    auto pts = vtkm::make_VecFromPortalPermute(&indices, this->Coords);
    vtkm::Vec3f pc;
    bool inside;
    auto status = this->PointInsideCell(point, this->CellSet.GetCellShape(cid), pts, pc, inside);
    if (status == vtkm::ErrorCode::Success && inside)
    {
      parametric = pc;
      return true;
    }

    return false;
  }

  vtkm::Id3 CellDims;
  vtkm::Vec3f Origin;
  vtkm::Vec3f MaxPoint;
  vtkm::Vec3f InvSpacing;
  vtkm::Id3 MaxCellIds;

  CellIdReadPortal CellIds;

  CellStructureType CellSet;
  CoordsPortalType Coords;
};

}
} // vtkm::exec

#endif //vtk_m_exec_CellLocatorUniformBins_h
