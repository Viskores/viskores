//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_CellSetSingleType_h
#define vtk_m_cont_CellSetSingleType_h

#include <vtkm/CellShape.h>
#include <vtkm/CellTraits.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/CellSetExplicit.h>

#include <map>
#include <utility>

namespace vtkm
{
namespace cont
{

//Only works with fixed sized cell sets

template <typename ConnectivityStorageTag = VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG>
class VTKM_ALWAYS_EXPORT CellSetSingleType
  : public vtkm::cont::CellSetExplicit<
      typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag, //ShapeStorageTag
      typename vtkm::cont::ArrayHandleConstant<
        vtkm::IdComponent>::StorageTag, //NumIndicesStorageTag
      ConnectivityStorageTag,
      typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag //IndexOffsetStorageTag
      >
{
  using Thisclass = vtkm::cont::CellSetSingleType<ConnectivityStorageTag>;
  using Superclass = vtkm::cont::CellSetExplicit<
    typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag,
    typename vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>::StorageTag,
    ConnectivityStorageTag,
    typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag>;

public:
  VTKM_CONT
  CellSetSingleType(const std::string& name = std::string())
    : Superclass(name)
    , ExpectedNumberOfCellsAdded(-1)
    , CellShapeAsId(CellShapeTagEmpty::Id)
    , NumberOfPointsPerCell(0)
  {
  }

  VTKM_CONT
  CellSetSingleType(const Thisclass& src)
    : Superclass(src)
    , ExpectedNumberOfCellsAdded(-1)
    , CellShapeAsId(src.CellShapeAsId)
    , NumberOfPointsPerCell(src.NumberOfPointsPerCell)
  {
  }

  VTKM_CONT
  Thisclass& operator=(const Thisclass& src)
  {
    this->Superclass::operator=(src);
    this->CellShapeAsId = src.CellShapeAsId;
    this->NumberOfPointsPerCell = src.NumberOfPointsPerCell;
    return *this;
  }

  virtual ~CellSetSingleType() {}

  /// First method to add cells -- one at a time.
  VTKM_CONT
  void PrepareToAddCells(vtkm::Id numCells, vtkm::Id connectivityMaxLen)
  {
    this->CellShapeAsId = vtkm::CELL_SHAPE_EMPTY;

    this->PointToCell.Connectivity.Allocate(connectivityMaxLen);

    this->NumberOfCellsAdded = 0;
    this->ConnectivityAdded = 0;
    this->ExpectedNumberOfCellsAdded = numCells;
  }

  /// Second method to add cells -- one at a time.
  template <typename IdVecType>
  VTKM_CONT void AddCell(vtkm::UInt8 shapeId, vtkm::IdComponent numVertices, const IdVecType& ids)
  {
    using Traits = vtkm::VecTraits<IdVecType>;
    VTKM_STATIC_ASSERT_MSG((std::is_same<typename Traits::ComponentType, vtkm::Id>::value),
                           "CellSetSingleType::AddCell requires vtkm::Id for indices.");

    if (Traits::GetNumberOfComponents(ids) < numVertices)
    {
      throw vtkm::cont::ErrorBadValue("Not enough indices given to CellSetSingleType::AddCell.");
    }

    if (this->ConnectivityAdded + numVertices > this->PointToCell.Connectivity.GetNumberOfValues())
    {
      throw vtkm::cont::ErrorBadValue(
        "Connectivity increased passed estimated maximum connectivity.");
    }

    if (this->CellShapeAsId == vtkm::CELL_SHAPE_EMPTY)
    {
      if (shapeId == vtkm::CELL_SHAPE_EMPTY)
      {
        throw vtkm::cont::ErrorBadValue("Cannot create cells of type empty.");
      }
      this->CellShapeAsId = shapeId;
      this->CheckNumberOfPointsPerCell(numVertices);
      this->NumberOfPointsPerCell = numVertices;
    }
    else
    {
      if (shapeId != this->GetCellShape(0))
      {
        throw vtkm::cont::ErrorBadValue("Cannot have differing shapes in CellSetSingleType.");
      }
      if (numVertices != this->NumberOfPointsPerCell)
      {
        throw vtkm::cont::ErrorBadValue(
          "Inconsistent number of points in cells for CellSetSingleType.");
      }
    }
    for (vtkm::IdComponent iVert = 0; iVert < numVertices; ++iVert)
    {
      this->PointToCell.Connectivity.GetPortalControl().Set(this->ConnectivityAdded + iVert,
                                                            Traits::GetComponent(ids, iVert));
    }
    this->NumberOfCellsAdded++;
    this->ConnectivityAdded += numVertices;
  }

  /// Third and final method to add cells -- one at a time.
  VTKM_CONT
  void CompleteAddingCells(vtkm::Id numPoints)
  {
    this->NumberOfPoints = numPoints;
    this->PointToCell.Connectivity.Shrink(this->ConnectivityAdded);

    vtkm::Id numCells = this->NumberOfCellsAdded;

    this->PointToCell.Shapes =
      vtkm::cont::make_ArrayHandleConstant(this->GetCellShape(0), numCells);
    this->PointToCell.NumIndices =
      vtkm::cont::make_ArrayHandleConstant(this->NumberOfPointsPerCell, numCells);
    this->PointToCell.IndexOffsets = vtkm::cont::make_ArrayHandleCounting(
      vtkm::Id(0), static_cast<vtkm::Id>(this->NumberOfPointsPerCell), numCells);

    this->PointToCell.ElementsValid = true;
    this->PointToCell.IndexOffsetsValid = true;

    if (this->ExpectedNumberOfCellsAdded != this->GetNumberOfCells())
    {
      throw vtkm::cont::ErrorBadValue("Did not add the expected number of cells.");
    }

    this->NumberOfCellsAdded = -1;
    this->ConnectivityAdded = -1;
    this->ExpectedNumberOfCellsAdded = -1;
  }

  //This is the way you can fill the memory from another system without copying
  VTKM_CONT
  void Fill(vtkm::Id numPoints,
            vtkm::UInt8 shapeId,
            vtkm::IdComponent numberOfPointsPerCell,
            const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag>& connectivity)
  {
    this->NumberOfPoints = numPoints;
    this->CellShapeAsId = shapeId;
    this->CheckNumberOfPointsPerCell(numberOfPointsPerCell);
    const vtkm::Id numCells = connectivity.GetNumberOfValues() / numberOfPointsPerCell;
    VTKM_ASSERT((connectivity.GetNumberOfValues() % numberOfPointsPerCell) == 0);
    this->PointToCell.Shapes = vtkm::cont::make_ArrayHandleConstant(shapeId, numCells);
    this->PointToCell.NumIndices =
      vtkm::cont::make_ArrayHandleConstant(numberOfPointsPerCell, numCells);
    this->PointToCell.IndexOffsets = vtkm::cont::make_ArrayHandleCounting(
      vtkm::Id(0), static_cast<vtkm::Id>(numberOfPointsPerCell), numCells);
    this->PointToCell.Connectivity = connectivity;

    this->PointToCell.ElementsValid = true;
    this->PointToCell.IndexOffsetsValid = true;
  }

  VTKM_CONT
  vtkm::Id GetCellShapeAsId() const { return this->CellShapeAsId; }

  VTKM_CONT
  vtkm::UInt8 GetCellShape(vtkm::Id vtkmNotUsed(cellIndex)) const
  {
    return static_cast<vtkm::UInt8>(this->CellShapeAsId);
  }

  virtual void PrintSummary(std::ostream& out) const
  {
    out << "   ExplicitSingleCellSet: " << this->Name << " Type " << this->CellShapeAsId
        << std::endl;
    out << "   PointToCell: " << std::endl;
    this->PointToCell.PrintSummary(out);
    out << "   CellToPoint: " << std::endl;
    this->CellToPoint.PrintSummary(out);
  }

private:
  template <typename CellShapeTag>
  void CheckNumberOfPointsPerCell(CellShapeTag,
                                  vtkm::CellTraitsTagSizeFixed,
                                  vtkm::IdComponent numVertices) const
  {
    if (numVertices != vtkm::CellTraits<CellShapeTag>::NUM_POINTS)
    {
      throw vtkm::cont::ErrorBadValue("Passed invalid number of points for cell shape.");
    }
  }

  template <typename CellShapeTag>
  void CheckNumberOfPointsPerCell(CellShapeTag,
                                  vtkm::CellTraitsTagSizeVariable,
                                  vtkm::IdComponent vtkmNotUsed(numVertices)) const
  {
    // Technically, a shape with a variable number of points probably has a
    // minimum number of points, but we are not being sophisticated enough to
    // check that. Instead, just pass the check by returning without error.
  }

  void CheckNumberOfPointsPerCell(vtkm::IdComponent numVertices) const
  {
    switch (this->CellShapeAsId)
    {
      vtkmGenericCellShapeMacro(this->CheckNumberOfPointsPerCell(
        CellShapeTag(), vtkm::CellTraits<CellShapeTag>::IsSizeFixed(), numVertices));
      default:
        throw vtkm::cont::ErrorBadValue("CellSetSingleType unable to determine the cell type");
    }
  }

  vtkm::Id ExpectedNumberOfCellsAdded;
  vtkm::Id CellShapeAsId;
  vtkm::IdComponent NumberOfPointsPerCell;
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetSingleType_h
