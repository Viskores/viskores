//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/TryExecute.h>

namespace vtkm
{
namespace cont
{

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT CellSetExplicit<ShapeStorageTag,
                          NumIndicesStorageTag,
                          ConnectivityStorageTag,
                          OffsetsStorageTag>::CellSetExplicit(const std::string& name)
  : CellSet(name)
  , ConnectivityAdded(-1)
  , NumberOfCellsAdded(-1)
  , NumberOfPoints(0)
{
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT CellSetExplicit<ShapeStorageTag,
                          NumIndicesStorageTag,
                          ConnectivityStorageTag,
                          OffsetsStorageTag>::CellSetExplicit(const Thisclass& src)
  : CellSet(src)
  , PointToCell(src.PointToCell)
  , CellToPoint(src.CellToPoint)
  , ConnectivityAdded(src.ConnectivityAdded)
  , NumberOfCellsAdded(src.NumberOfCellsAdded)
  , NumberOfPoints(src.NumberOfPoints)
{
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT auto
CellSetExplicit<ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>::
operator=(const Thisclass& src) -> Thisclass&
{
  this->CellSet::operator=(src);
  this->PointToCell = src.PointToCell;
  this->CellToPoint = src.CellToPoint;
  this->ConnectivityAdded = src.ConnectivityAdded;
  this->NumberOfCellsAdded = src.NumberOfCellsAdded;
  this->NumberOfPoints = src.NumberOfPoints;
  return *this;
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
CellSetExplicit<ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>::
  ~CellSetExplicit()
// explicitly define instead of '=default' to workaround an intel compiler bug
// (see #179)
{
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
void CellSetExplicit<ShapeStorageTag,
                     NumIndicesStorageTag,
                     ConnectivityStorageTag,
                     OffsetsStorageTag>::PrintSummary(std::ostream& out) const
{
  out << "   ExplicitCellSet: " << this->Name << std::endl;
  out << "   PointToCell: " << std::endl;
  this->PointToCell.PrintSummary(out);
  out << "   CellToPoint: " << std::endl;
  this->CellToPoint.PrintSummary(out);
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetStorageTag>
void CellSetExplicit<ShapeStorageTag,
                     NumIndicesStorageTag,
                     ConnectivityStorageTag,
                     OffsetStorageTag>::ReleaseResourcesExecution()
{
  this->PointToCell.ReleaseResourcesExecution();
  this->CellToPoint.ReleaseResourcesExecution();
}

//----------------------------------------------------------------------------

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
vtkm::Id CellSetExplicit<ShapeStorageTag,
                         NumIndicesStorageTag,
                         ConnectivityStorageTag,
                         OffsetsStorageTag>::GetNumberOfCells() const
{
  return this->PointToCell.GetNumberOfElements();
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
vtkm::Id CellSetExplicit<ShapeStorageTag,
                         NumIndicesStorageTag,
                         ConnectivityStorageTag,
                         OffsetsStorageTag>::GetNumberOfPoints() const
{
  return this->NumberOfPoints;
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
vtkm::Id CellSetExplicit<ShapeStorageTag,
                         NumIndicesStorageTag,
                         ConnectivityStorageTag,
                         OffsetsStorageTag>::GetNumberOfFaces() const
{
  return -1;
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
vtkm::Id CellSetExplicit<ShapeStorageTag,
                         NumIndicesStorageTag,
                         ConnectivityStorageTag,
                         OffsetsStorageTag>::GetNumberOfEdges() const
{
  return -1;
}

//----------------------------------------------------------------------------

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT vtkm::Id
  CellSetExplicit<ShapeStorageTag,
                  NumIndicesStorageTag,
                  ConnectivityStorageTag,
                  OffsetsStorageTag>::GetSchedulingRange(vtkm::TopologyElementTagCell) const
{
  return this->GetNumberOfCells();
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT vtkm::Id
  CellSetExplicit<ShapeStorageTag,
                  NumIndicesStorageTag,
                  ConnectivityStorageTag,
                  OffsetsStorageTag>::GetSchedulingRange(vtkm::TopologyElementTagPoint) const
{
  return this->GetNumberOfPoints();
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT vtkm::IdComponent
CellSetExplicit<ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>::
  GetNumberOfPointsInCell(vtkm::Id cellIndex) const
{
  return this->PointToCell.NumIndices.GetPortalConstControl().Get(cellIndex);
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT vtkm::UInt8 CellSetExplicit<ShapeStorageTag,
                                      NumIndicesStorageTag,
                                      ConnectivityStorageTag,
                                      OffsetsStorageTag>::GetCellShape(vtkm::Id cellIndex) const
{
  return this->PointToCell.Shapes.GetPortalConstControl().Get(cellIndex);
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
template <vtkm::IdComponent ItemTupleLength>
VTKM_CONT void
CellSetExplicit<ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>::
  GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id, ItemTupleLength>& ids) const
{
  this->PointToCell.BuildIndexOffsets(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
  vtkm::IdComponent numIndices = this->GetNumberOfPointsInCell(index);
  vtkm::Id start = this->PointToCell.IndexOffsets.GetPortalConstControl().Get(index);
  for (vtkm::IdComponent i = 0; i < numIndices && i < ItemTupleLength; i++)
  {
    ids[i] = this->PointToCell.Connectivity.GetPortalConstControl().Get(start + i);
  }
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT void
CellSetExplicit<ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>::
  GetIndices(vtkm::Id index, vtkm::cont::ArrayHandle<vtkm::Id>& ids) const
{
  this->PointToCell.BuildIndexOffsets(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
  vtkm::IdComponent numIndices = this->GetNumberOfPointsInCell(index);
  ids.Allocate(numIndices);
  vtkm::Id start = this->PointToCell.IndexOffsets.GetPortalConstControl().Get(index);
  vtkm::cont::ArrayHandle<vtkm::Id>::PortalControl idPortal = ids.GetPortalControl();
  auto PtCellPortal = this->PointToCell.Connectivity.GetPortalConstControl();

  for (vtkm::IdComponent i = 0; i < numIndices && i < numIndices; i++)
    idPortal.Set(i, PtCellPortal.Get(start + i));
}

//----------------------------------------------------------------------------

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT void CellSetExplicit<ShapeStorageTag,
                               NumIndicesStorageTag,
                               ConnectivityStorageTag,
                               OffsetsStorageTag>::PrepareToAddCells(vtkm::Id numCells,
                                                                     vtkm::Id connectivityMaxLen)
{
  this->PointToCell.Shapes.Allocate(numCells);
  this->PointToCell.NumIndices.Allocate(numCells);
  this->PointToCell.Connectivity.Allocate(connectivityMaxLen);
  this->PointToCell.IndexOffsets.Allocate(numCells);
  this->NumberOfCellsAdded = 0;
  this->ConnectivityAdded = 0;
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
template <typename IdVecType>
VTKM_CONT void
CellSetExplicit<ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>::
  AddCell(vtkm::UInt8 cellType, vtkm::IdComponent numVertices, const IdVecType& ids)
{
  using Traits = vtkm::VecTraits<IdVecType>;
  VTKM_STATIC_ASSERT_MSG((std::is_same<typename Traits::ComponentType, vtkm::Id>::value),
                         "CellSetSingleType::AddCell requires vtkm::Id for indices.");

  if (Traits::GetNumberOfComponents(ids) < numVertices)
  {
    throw vtkm::cont::ErrorBadValue("Not enough indices given to CellSetSingleType::AddCell.");
  }

  if (this->NumberOfCellsAdded >= this->PointToCell.Shapes.GetNumberOfValues())
  {
    throw vtkm::cont::ErrorBadValue("Added more cells then expected.");
  }
  if (this->ConnectivityAdded + numVertices > this->PointToCell.Connectivity.GetNumberOfValues())
  {
    throw vtkm::cont::ErrorBadValue(
      "Connectivity increased passed estimated maximum connectivity.");
  }

  this->PointToCell.Shapes.GetPortalControl().Set(this->NumberOfCellsAdded, cellType);
  this->PointToCell.NumIndices.GetPortalControl().Set(this->NumberOfCellsAdded, numVertices);
  for (vtkm::IdComponent iVec = 0; iVec < numVertices; ++iVec)
  {
    this->PointToCell.Connectivity.GetPortalControl().Set(this->ConnectivityAdded + iVec,
                                                          Traits::GetComponent(ids, iVec));
  }
  this->PointToCell.IndexOffsets.GetPortalControl().Set(this->NumberOfCellsAdded,
                                                        this->ConnectivityAdded);
  this->NumberOfCellsAdded++;
  this->ConnectivityAdded += numVertices;
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT void CellSetExplicit<ShapeStorageTag,
                               NumIndicesStorageTag,
                               ConnectivityStorageTag,
                               OffsetsStorageTag>::CompleteAddingCells(vtkm::Id numPoints)
{
  this->NumberOfPoints = numPoints;
  this->PointToCell.Connectivity.Shrink(ConnectivityAdded);
  this->PointToCell.ElementsValid = true;
  this->PointToCell.IndexOffsetsValid = true;

  if (this->NumberOfCellsAdded != this->GetNumberOfCells())
  {
    throw vtkm::cont::ErrorBadValue("Did not add as many cells as expected.");
  }

  this->NumberOfCellsAdded = -1;
  this->ConnectivityAdded = -1;
}

//----------------------------------------------------------------------------

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT void
CellSetExplicit<ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>::
  Fill(vtkm::Id numPoints,
       const vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorageTag>& cellTypes,
       const vtkm::cont::ArrayHandle<vtkm::IdComponent, NumIndicesStorageTag>& numIndices,
       const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag>& connectivity,
       const vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorageTag>& offsets)
{
  this->NumberOfPoints = numPoints;
  this->PointToCell.Shapes = cellTypes;
  this->PointToCell.NumIndices = numIndices;
  this->PointToCell.Connectivity = connectivity;

  this->PointToCell.ElementsValid = true;

  if (offsets.GetNumberOfValues() == cellTypes.GetNumberOfValues())
  {
    this->PointToCell.IndexOffsets = offsets;
    this->PointToCell.IndexOffsetsValid = true;
  }
  else
  {
    this->PointToCell.IndexOffsetsValid = false;
    if (offsets.GetNumberOfValues() != 0)
    {
      throw vtkm::cont::ErrorBadValue("Explicit cell offsets array unexpected size. "
                                      "Use an empty array to automatically generate.");
    }
  }
}

//----------------------------------------------------------------------------

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
template <typename Device, typename FromTopology, typename ToTopology>
auto CellSetExplicit<ShapeStorageTag,
                     NumIndicesStorageTag,
                     ConnectivityStorageTag,
                     OffsetsStorageTag>::PrepareForInput(Device, FromTopology, ToTopology) const ->
  typename ExecutionTypes<Device, FromTopology, ToTopology>::ExecObjectType
{
  this->BuildConnectivity(FromTopology(), ToTopology());

  const auto& connectivity = this->GetConnectivity(FromTopology(), ToTopology());
  VTKM_ASSERT(connectivity.ElementsValid);

  using ExecObjType = typename ExecutionTypes<Device, FromTopology, ToTopology>::ExecObjectType;
  return ExecObjType(connectivity.Shapes.PrepareForInput(Device()),
                     connectivity.NumIndices.PrepareForInput(Device()),
                     connectivity.Connectivity.PrepareForInput(Device()),
                     connectivity.IndexOffsets.PrepareForInput(Device()));
}

//----------------------------------------------------------------------------

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
template <typename FromTopology, typename ToTopology>
VTKM_CONT auto CellSetExplicit<ShapeStorageTag,
                               NumIndicesStorageTag,
                               ConnectivityStorageTag,
                               OffsetsStorageTag>::GetShapesArray(FromTopology, ToTopology) const
  -> const typename ConnectivityChooser<FromTopology, ToTopology>::ShapeArrayType&
{
  this->BuildConnectivity(FromTopology(), ToTopology());
  return this->GetConnectivity(FromTopology(), ToTopology()).Shapes;
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
template <typename FromTopology, typename ToTopology>
VTKM_CONT auto CellSetExplicit<ShapeStorageTag,
                               NumIndicesStorageTag,
                               ConnectivityStorageTag,
                               OffsetsStorageTag>::GetNumIndicesArray(FromTopology,
                                                                      ToTopology) const -> const
  typename ConnectivityChooser<FromTopology, ToTopology>::NumIndicesArrayType&
{
  this->BuildConnectivity(FromTopology(), ToTopology());
  return this->GetConnectivity(FromTopology(), ToTopology()).NumIndices;
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
template <typename FromTopology, typename ToTopology>
VTKM_CONT auto CellSetExplicit<ShapeStorageTag,
                               NumIndicesStorageTag,
                               ConnectivityStorageTag,
                               OffsetsStorageTag>::GetConnectivityArray(FromTopology,
                                                                        ToTopology) const -> const
  typename ConnectivityChooser<FromTopology, ToTopology>::ConnectivityArrayType&
{
  this->BuildConnectivity(FromTopology(), ToTopology());
  return this->GetConnectivity(FromTopology(), ToTopology()).Connectivity;
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
template <typename FromTopology, typename ToTopology>
VTKM_CONT auto CellSetExplicit<ShapeStorageTag,
                               NumIndicesStorageTag,
                               ConnectivityStorageTag,
                               OffsetsStorageTag>::GetIndexOffsetArray(FromTopology,
                                                                       ToTopology) const -> const
  typename ConnectivityChooser<FromTopology, ToTopology>::IndexOffsetArrayType&
{
  this->BuildConnectivity(FromTopology(), ToTopology());
  return this->GetConnectivity(FromTopology(), ToTopology()).IndexOffsets;
}

//----------------------------------------------------------------------------

namespace detail
{

template <typename PointToCellConnectivity>
struct BuildPointToCellConnectivityFunctor
{
  explicit BuildPointToCellConnectivityFunctor(PointToCellConnectivity& pointToCell)
    : PointToCell(&pointToCell)
  {
  }

  template <typename Device>
  bool operator()(Device) const
  {
    this->PointToCell->BuildIndexOffsets(Device());
    return true;
  }

  PointToCellConnectivity* PointToCell;
};

template <typename PointToCellConnectivity, typename CellToPointConnectivity>
struct BuildCellToPointConnectivityFunctor
{
  BuildCellToPointConnectivityFunctor(PointToCellConnectivity& pointToCell,
                                      CellToPointConnectivity& cellToPoint,
                                      vtkm::Id numberOfPoints)
    : PointToCell(&pointToCell)
    , CellToPoint(&cellToPoint)
    , NumberOfPoints(numberOfPoints)
  {
  }

  template <typename Device>
  bool operator()(Device) const
  {
    this->PointToCell->BuildIndexOffsets(Device());
    internal::ComputeCellToPointConnectivity(
      *this->CellToPoint, *this->PointToCell, this->NumberOfPoints, Device());
    this->CellToPoint->BuildIndexOffsets(Device());
    return true;
  }

  PointToCellConnectivity* PointToCell;
  CellToPointConnectivity* CellToPoint;
  vtkm::Id NumberOfPoints;
};

} // detail

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT void
CellSetExplicit<ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>::
  BuildConnectivity(vtkm::TopologyElementTagPoint,
                    vtkm::TopologyElementTagCell,
                    vtkm::cont::RuntimeDeviceTracker tracker) const
{
  using PointToCellConnectivity =
    typename ConnectivityChooser<vtkm::TopologyElementTagPoint,
                                 vtkm::TopologyElementTagCell>::ConnectivityType;

  VTKM_ASSERT(this->PointToCell.ElementsValid);
  if (!this->PointToCell.IndexOffsetsValid)
  {
    auto self = const_cast<Thisclass*>(this);
    auto functor =
      detail::BuildPointToCellConnectivityFunctor<PointToCellConnectivity>(self->PointToCell);
    if (!vtkm::cont::TryExecute(functor, tracker))
    {
      throw vtkm::cont::ErrorExecution("Failed to run BuildConnectivity on any device.");
    }
  }
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT void
CellSetExplicit<ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>::
  BuildConnectivity(vtkm::TopologyElementTagCell,
                    vtkm::TopologyElementTagPoint,
                    vtkm::cont::RuntimeDeviceTracker tracker) const
{
  using PointToCellConnectivity =
    typename ConnectivityChooser<vtkm::TopologyElementTagPoint,
                                 vtkm::TopologyElementTagCell>::ConnectivityType;
  using CellToPointConnectivity =
    typename ConnectivityChooser<vtkm::TopologyElementTagCell,
                                 vtkm::TopologyElementTagPoint>::ConnectivityType;

  if (!this->CellToPoint.ElementsValid || !this->CellToPoint.IndexOffsetsValid)
  {
    auto self = const_cast<Thisclass*>(this);
    auto functor =
      detail::BuildCellToPointConnectivityFunctor<PointToCellConnectivity, CellToPointConnectivity>(
        self->PointToCell, self->CellToPoint, this->NumberOfPoints);
    if (!vtkm::cont::TryExecute(functor, tracker))
    {
      throw vtkm::cont::ErrorExecution("Failed to run BuildConnectivity on any device.");
    }
  }
}
}
} // vtkm::cont
