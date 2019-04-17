//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
  , Data(std::make_shared<Internals>())
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
  , Data(src.Data)
{
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT CellSetExplicit<ShapeStorageTag,
                          NumIndicesStorageTag,
                          ConnectivityStorageTag,
                          OffsetsStorageTag>::CellSetExplicit(Thisclass &&src) noexcept
  : CellSet(std::forward<CellSet>(src)),
    Data(std::move(src.Data))
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
  this->Data = src.Data;
  return *this;
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT auto
CellSetExplicit<ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>::
operator=(Thisclass&& src) noexcept -> Thisclass&
{
  this->CellSet::operator=(std::forward<CellSet>(src));
  this->Data = std::move(src.Data);
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
  this->Data->PointToCell.PrintSummary(out);
  out << "   CellToPoint: " << std::endl;
  this->Data->CellToPoint.PrintSummary(out);
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
  this->Data->PointToCell.ReleaseResourcesExecution();
  this->Data->CellToPoint.ReleaseResourcesExecution();
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
  return this->Data->PointToCell.GetNumberOfElements();
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
  return this->Data->NumberOfPoints;
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
void CellSetExplicit<ShapeStorageTag,
                     NumIndicesStorageTag,
                     ConnectivityStorageTag,
                     OffsetsStorageTag>::GetCellPointIds(vtkm::Id id, vtkm::Id* ptids) const
{
  auto arrayWrapper = vtkm::cont::make_ArrayHandle(ptids, this->GetNumberOfPointsInCell(id));
  this->GetIndices(id, arrayWrapper);
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
  return this->Data->PointToCell.NumIndices.GetPortalConstControl().Get(cellIndex);
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
  return this->Data->PointToCell.Shapes.GetPortalConstControl().Get(cellIndex);
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
  this->Data->PointToCell.BuildIndexOffsets(vtkm::cont::DeviceAdapterTagAny{});
  vtkm::IdComponent numIndices = this->GetNumberOfPointsInCell(index);
  vtkm::Id start = this->Data->PointToCell.IndexOffsets.GetPortalConstControl().Get(index);
  for (vtkm::IdComponent i = 0; i < numIndices && i < ItemTupleLength; i++)
  {
    ids[i] = this->Data->PointToCell.Connectivity.GetPortalConstControl().Get(start + i);
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
  this->Data->PointToCell.BuildIndexOffsets(vtkm::cont::DeviceAdapterTagAny{});
  vtkm::IdComponent numIndices = this->GetNumberOfPointsInCell(index);
  ids.Allocate(numIndices);
  vtkm::Id start = this->Data->PointToCell.IndexOffsets.GetPortalConstControl().Get(index);
  vtkm::cont::ArrayHandle<vtkm::Id>::PortalControl idPortal = ids.GetPortalControl();
  auto PtCellPortal = this->Data->PointToCell.Connectivity.GetPortalConstControl();

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
  this->Data->PointToCell.Shapes.Allocate(numCells);
  this->Data->PointToCell.NumIndices.Allocate(numCells);
  this->Data->PointToCell.Connectivity.Allocate(connectivityMaxLen);
  this->Data->PointToCell.IndexOffsets.Allocate(numCells);
  this->Data->NumberOfCellsAdded = 0;
  this->Data->ConnectivityAdded = 0;
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

  if (this->Data->NumberOfCellsAdded >= this->Data->PointToCell.Shapes.GetNumberOfValues())
  {
    throw vtkm::cont::ErrorBadValue("Added more cells then expected.");
  }
  if (this->Data->ConnectivityAdded + numVertices >
      this->Data->PointToCell.Connectivity.GetNumberOfValues())
  {
    throw vtkm::cont::ErrorBadValue(
      "Connectivity increased passed estimated maximum connectivity.");
  }

  this->Data->PointToCell.Shapes.GetPortalControl().Set(this->Data->NumberOfCellsAdded, cellType);
  this->Data->PointToCell.NumIndices.GetPortalControl().Set(this->Data->NumberOfCellsAdded,
                                                            numVertices);
  for (vtkm::IdComponent iVec = 0; iVec < numVertices; ++iVec)
  {
    this->Data->PointToCell.Connectivity.GetPortalControl().Set(
      this->Data->ConnectivityAdded + iVec, Traits::GetComponent(ids, iVec));
  }
  this->Data->PointToCell.IndexOffsets.GetPortalControl().Set(this->Data->NumberOfCellsAdded,
                                                              this->Data->ConnectivityAdded);
  this->Data->NumberOfCellsAdded++;
  this->Data->ConnectivityAdded += numVertices;
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
  this->Data->NumberOfPoints = numPoints;
  this->Data->PointToCell.Connectivity.Shrink(this->Data->ConnectivityAdded);
  this->Data->PointToCell.ElementsValid = true;
  this->Data->PointToCell.IndexOffsetsValid = true;

  if (this->Data->NumberOfCellsAdded != this->GetNumberOfCells())
  {
    throw vtkm::cont::ErrorBadValue("Did not add as many cells as expected.");
  }

  this->Data->NumberOfCellsAdded = -1;
  this->Data->ConnectivityAdded = -1;
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
  this->Data->NumberOfPoints = numPoints;
  this->Data->PointToCell.Shapes = cellTypes;
  this->Data->PointToCell.NumIndices = numIndices;
  this->Data->PointToCell.Connectivity = connectivity;

  this->Data->PointToCell.ElementsValid = true;

  if (offsets.GetNumberOfValues() == cellTypes.GetNumberOfValues())
  {
    this->Data->PointToCell.IndexOffsets = offsets;
    this->Data->PointToCell.IndexOffsetsValid = true;
  }
  else
  {
    this->Data->PointToCell.IndexOffsetsValid = false;
    if (offsets.GetNumberOfValues() != 0)
    {
      throw vtkm::cont::ErrorBadValue("Explicit cell offsets array unexpected size. "
                                      "Use an empty array to automatically generate.");
    }
  }

  this->ResetConnectivity(TopologyElementTagCell{}, TopologyElementTagPoint{});
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
  this->BuildConnectivity(Device{}, FromTopology(), ToTopology());

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
  this->BuildConnectivity(vtkm::cont::DeviceAdapterTagAny{}, FromTopology(), ToTopology());
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
  this->BuildConnectivity(vtkm::cont::DeviceAdapterTagAny{}, FromTopology(), ToTopology());
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
  this->BuildConnectivity(vtkm::cont::DeviceAdapterTagAny{}, FromTopology(), ToTopology());
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
  this->BuildConnectivity(vtkm::cont::DeviceAdapterTagAny{}, FromTopology(), ToTopology());
  return this->GetConnectivity(FromTopology(), ToTopology()).IndexOffsets;
}

//----------------------------------------------------------------------------

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
std::shared_ptr<CellSet> CellSetExplicit<ShapeStorageTag,
                                         NumIndicesStorageTag,
                                         ConnectivityStorageTag,
                                         OffsetsStorageTag>::NewInstance() const
{
  return std::make_shared<CellSetExplicit>();
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
void CellSetExplicit<ShapeStorageTag,
                     NumIndicesStorageTag,
                     ConnectivityStorageTag,
                     OffsetsStorageTag>::DeepCopy(const CellSet* src)
{
  const auto* other = dynamic_cast<const CellSetExplicit*>(src);
  if (!other)
  {
    throw vtkm::cont::ErrorBadType("CellSetExplicit::DeepCopy types don't match");
  }

  // TODO: implement actual deep-copy of the arrays
  auto pt = vtkm::TopologyElementTagPoint{};
  auto ct = vtkm::TopologyElementTagCell{};
  this->Fill(other->GetNumberOfPoints(),
             other->GetShapesArray(pt, ct),
             other->GetNumIndicesArray(pt, ct),
             other->GetConnectivityArray(pt, ct),
             other->GetIndexOffsetArray(pt, ct));
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
  BuildConnectivity(vtkm::cont::DeviceAdapterId device,
                    vtkm::TopologyElementTagPoint,
                    vtkm::TopologyElementTagCell) const
{
  using PointToCellConnectivity =
    typename ConnectivityChooser<vtkm::TopologyElementTagPoint,
                                 vtkm::TopologyElementTagCell>::ConnectivityType;

  VTKM_ASSERT(this->Data->PointToCell.ElementsValid);
  if (!this->Data->PointToCell.IndexOffsetsValid)
  {
    auto self = const_cast<Thisclass*>(this);
    auto functor =
      detail::BuildPointToCellConnectivityFunctor<PointToCellConnectivity>(self->Data->PointToCell);
    if (!vtkm::cont::TryExecuteOnDevice(device, functor))
    {
      throw vtkm::cont::ErrorExecution("Failed to run BuildConnectivity.");
    }
  }
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
VTKM_CONT void
CellSetExplicit<ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>::
  BuildConnectivity(vtkm::cont::DeviceAdapterId device,
                    vtkm::TopologyElementTagCell,
                    vtkm::TopologyElementTagPoint) const
{
  using PointToCellConnectivity =
    typename ConnectivityChooser<vtkm::TopologyElementTagPoint,
                                 vtkm::TopologyElementTagCell>::ConnectivityType;
  using CellToPointConnectivity =
    typename ConnectivityChooser<vtkm::TopologyElementTagCell,
                                 vtkm::TopologyElementTagPoint>::ConnectivityType;

  if (!this->Data->CellToPoint.ElementsValid || !this->Data->CellToPoint.IndexOffsetsValid)
  {
    auto self = const_cast<Thisclass*>(this);
    auto functor =
      detail::BuildCellToPointConnectivityFunctor<PointToCellConnectivity, CellToPointConnectivity>(
        self->Data->PointToCell, self->Data->CellToPoint, this->Data->NumberOfPoints);
    if (!vtkm::cont::TryExecuteOnDevice(device, functor))
    {
      throw vtkm::cont::ErrorExecution("Failed to run BuildConnectivity.");
    }
  }
}
}
} // vtkm::cont
