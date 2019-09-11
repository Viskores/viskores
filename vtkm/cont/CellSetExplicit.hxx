//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellSetExplicit_hxx
#define vtk_m_cont_CellSetExplicit_hxx

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
                          OffsetsStorageTag>::CellSetExplicit()
  : CellSet()
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
  out << "   ExplicitCellSet: " << std::endl;
  out << "   VisitCellsWithPoints: " << std::endl;
  this->Data->VisitCellsWithPoints.PrintSummary(out);
  out << "   VisitPointsWithCells: " << std::endl;
  this->Data->VisitPointsWithCells.PrintSummary(out);
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
  this->Data->VisitCellsWithPoints.ReleaseResourcesExecution();
  this->Data->VisitPointsWithCells.ReleaseResourcesExecution();
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
  return this->Data->VisitCellsWithPoints.GetNumberOfElements();
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
  return this->Data->VisitCellsWithPoints.NumIndices.GetPortalConstControl().Get(cellIndex);
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
  return this->Data->VisitCellsWithPoints.Shapes.GetPortalConstControl().Get(cellIndex);
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
  this->Data->VisitCellsWithPoints.BuildIndexOffsets(vtkm::cont::DeviceAdapterTagAny{});
  vtkm::IdComponent numIndices = this->GetNumberOfPointsInCell(index);
  vtkm::Id start = this->Data->VisitCellsWithPoints.IndexOffsets.GetPortalConstControl().Get(index);
  for (vtkm::IdComponent i = 0; i < numIndices && i < ItemTupleLength; i++)
  {
    ids[i] = this->Data->VisitCellsWithPoints.Connectivity.GetPortalConstControl().Get(start + i);
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
  this->Data->VisitCellsWithPoints.BuildIndexOffsets(vtkm::cont::DeviceAdapterTagAny{});
  vtkm::IdComponent numIndices = this->GetNumberOfPointsInCell(index);
  ids.Allocate(numIndices);
  vtkm::Id start = this->Data->VisitCellsWithPoints.IndexOffsets.GetPortalConstControl().Get(index);
  vtkm::cont::ArrayHandle<vtkm::Id>::PortalControl idPortal = ids.GetPortalControl();
  auto PtCellPortal = this->Data->VisitCellsWithPoints.Connectivity.GetPortalConstControl();

  for (vtkm::IdComponent i = 0; i < numIndices && i < numIndices; i++)
  {
    idPortal.Set(i, PtCellPortal.Get(start + i));
  }
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
  this->Data->VisitCellsWithPoints.Shapes.Allocate(numCells);
  this->Data->VisitCellsWithPoints.NumIndices.Allocate(numCells);
  this->Data->VisitCellsWithPoints.Connectivity.Allocate(connectivityMaxLen);
  this->Data->VisitCellsWithPoints.IndexOffsets.Allocate(numCells);
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

  if (this->Data->NumberOfCellsAdded >= this->Data->VisitCellsWithPoints.Shapes.GetNumberOfValues())
  {
    throw vtkm::cont::ErrorBadValue("Added more cells then expected.");
  }
  if (this->Data->ConnectivityAdded + numVertices >
      this->Data->VisitCellsWithPoints.Connectivity.GetNumberOfValues())
  {
    throw vtkm::cont::ErrorBadValue(
      "Connectivity increased passed estimated maximum connectivity.");
  }

  this->Data->VisitCellsWithPoints.Shapes.GetPortalControl().Set(this->Data->NumberOfCellsAdded,
                                                                 cellType);
  this->Data->VisitCellsWithPoints.NumIndices.GetPortalControl().Set(this->Data->NumberOfCellsAdded,
                                                                     numVertices);
  for (vtkm::IdComponent iVec = 0; iVec < numVertices; ++iVec)
  {
    this->Data->VisitCellsWithPoints.Connectivity.GetPortalControl().Set(
      this->Data->ConnectivityAdded + iVec, Traits::GetComponent(ids, iVec));
  }
  this->Data->VisitCellsWithPoints.IndexOffsets.GetPortalControl().Set(
    this->Data->NumberOfCellsAdded, this->Data->ConnectivityAdded);
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
  this->Data->VisitCellsWithPoints.Connectivity.Shrink(this->Data->ConnectivityAdded);
  this->Data->VisitCellsWithPoints.ElementsValid = true;
  this->Data->VisitCellsWithPoints.IndexOffsetsValid = true;

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
  this->Data->VisitCellsWithPoints.Shapes = cellTypes;
  this->Data->VisitCellsWithPoints.NumIndices = numIndices;
  this->Data->VisitCellsWithPoints.Connectivity = connectivity;

  this->Data->VisitCellsWithPoints.ElementsValid = true;

  if (offsets.GetNumberOfValues() == cellTypes.GetNumberOfValues())
  {
    this->Data->VisitCellsWithPoints.IndexOffsets = offsets;
    this->Data->VisitCellsWithPoints.IndexOffsetsValid = true;
  }
  else
  {
    this->Data->VisitCellsWithPoints.IndexOffsetsValid = false;
    if (offsets.GetNumberOfValues() != 0)
    {
      throw vtkm::cont::ErrorBadValue("Explicit cell offsets array unexpected size. "
                                      "Use an empty array to automatically generate.");
    }
  }

  this->ResetConnectivity(TopologyElementTagPoint{}, TopologyElementTagCell{});
}

//----------------------------------------------------------------------------

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
template <typename Device, typename VisitTopology, typename IncidentTopology>
auto CellSetExplicit<ShapeStorageTag,
                     NumIndicesStorageTag,
                     ConnectivityStorageTag,
                     OffsetsStorageTag>::PrepareForInput(Device,
                                                         VisitTopology,
                                                         IncidentTopology) const ->
  typename ExecutionTypes<Device, VisitTopology, IncidentTopology>::ExecObjectType
{
  this->BuildConnectivity(Device{}, VisitTopology(), IncidentTopology());

  const auto& connectivity = this->GetConnectivity(VisitTopology(), IncidentTopology());
  VTKM_ASSERT(connectivity.ElementsValid);

  using ExecObjType =
    typename ExecutionTypes<Device, VisitTopology, IncidentTopology>::ExecObjectType;
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
template <typename VisitTopology, typename IncidentTopology>
VTKM_CONT auto CellSetExplicit<ShapeStorageTag,
                               NumIndicesStorageTag,
                               ConnectivityStorageTag,
                               OffsetsStorageTag>::GetShapesArray(VisitTopology,
                                                                  IncidentTopology) const -> const
  typename ConnectivityChooser<VisitTopology, IncidentTopology>::ShapeArrayType&
{
  this->BuildConnectivity(vtkm::cont::DeviceAdapterTagAny{}, VisitTopology(), IncidentTopology());
  return this->GetConnectivity(VisitTopology(), IncidentTopology()).Shapes;
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
template <typename VisitTopology, typename IncidentTopology>
VTKM_CONT auto CellSetExplicit<ShapeStorageTag,
                               NumIndicesStorageTag,
                               ConnectivityStorageTag,
                               OffsetsStorageTag>::GetNumIndicesArray(VisitTopology,
                                                                      IncidentTopology) const
  -> const typename ConnectivityChooser<VisitTopology, IncidentTopology>::NumIndicesArrayType&
{
  this->BuildConnectivity(vtkm::cont::DeviceAdapterTagAny{}, VisitTopology(), IncidentTopology());
  return this->GetConnectivity(VisitTopology(), IncidentTopology()).NumIndices;
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
template <typename VisitTopology, typename IncidentTopology>
VTKM_CONT auto CellSetExplicit<ShapeStorageTag,
                               NumIndicesStorageTag,
                               ConnectivityStorageTag,
                               OffsetsStorageTag>::GetConnectivityArray(VisitTopology,
                                                                        IncidentTopology) const
  -> const typename ConnectivityChooser<VisitTopology, IncidentTopology>::ConnectivityArrayType&
{
  this->BuildConnectivity(vtkm::cont::DeviceAdapterTagAny{}, VisitTopology(), IncidentTopology());
  return this->GetConnectivity(VisitTopology(), IncidentTopology()).Connectivity;
}

template <typename ShapeStorageTag,
          typename NumIndicesStorageTag,
          typename ConnectivityStorageTag,
          typename OffsetsStorageTag>
template <typename VisitTopology, typename IncidentTopology>
VTKM_CONT auto CellSetExplicit<ShapeStorageTag,
                               NumIndicesStorageTag,
                               ConnectivityStorageTag,
                               OffsetsStorageTag>::GetIndexOffsetArray(VisitTopology,
                                                                       IncidentTopology) const
  -> const typename ConnectivityChooser<VisitTopology, IncidentTopology>::IndexOffsetArrayType&
{
  this->BuildConnectivity(vtkm::cont::DeviceAdapterTagAny{}, VisitTopology(), IncidentTopology());
  return this->GetConnectivity(VisitTopology(), IncidentTopology()).IndexOffsets;
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
  auto ct = vtkm::TopologyElementTagCell{};
  auto pt = vtkm::TopologyElementTagPoint{};
  this->Fill(other->GetNumberOfPoints(),
             other->GetShapesArray(ct, pt),
             other->GetNumIndicesArray(ct, pt),
             other->GetConnectivityArray(ct, pt),
             other->GetIndexOffsetArray(ct, pt));
}

//----------------------------------------------------------------------------

namespace detail
{

template <typename VisitCellsWithPointsConnectivity>
struct BuildVisitCellsWithPointsConnectivityFunctor
{
  explicit BuildVisitCellsWithPointsConnectivityFunctor(VisitCellsWithPointsConnectivity& obj)
    : VisitCellsWithPoints(&obj)
  {
  }

  template <typename Device>
  bool operator()(Device) const
  {
    this->VisitCellsWithPoints->BuildIndexOffsets(Device());
    return true;
  }

  VisitCellsWithPointsConnectivity* VisitCellsWithPoints;
};

template <typename VisitCellsWithPointsConnectivity, typename VisitPointsWithCellsConnectivity>
struct BuildVisitPointsWithCellsConnectivityFunctor
{
  BuildVisitPointsWithCellsConnectivityFunctor(
    VisitCellsWithPointsConnectivity& visitCellsWithPoints,
    VisitPointsWithCellsConnectivity& visitPointsWithCells,
    vtkm::Id numberOfPoints)
    : VisitCellsWithPoints(&visitCellsWithPoints)
    , VisitPointsWithCells(&visitPointsWithCells)
    , NumberOfPoints(numberOfPoints)
  {
  }

  template <typename Device>
  bool operator()(Device) const
  {
    this->VisitCellsWithPoints->BuildIndexOffsets(Device());
    internal::ComputeVisitPointsWithCellsConnectivity(
      *this->VisitPointsWithCells, *this->VisitCellsWithPoints, this->NumberOfPoints, Device());
    this->VisitPointsWithCells->BuildIndexOffsets(Device());
    return true;
  }

  VisitCellsWithPointsConnectivity* VisitCellsWithPoints;
  VisitPointsWithCellsConnectivity* VisitPointsWithCells;
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
                    vtkm::TopologyElementTagCell,
                    vtkm::TopologyElementTagPoint) const
{
  using VisitCellsWithPointsConnectivity =
    typename ConnectivityChooser<vtkm::TopologyElementTagCell,
                                 vtkm::TopologyElementTagPoint>::ConnectivityType;

  VTKM_ASSERT(this->Data->VisitCellsWithPoints.ElementsValid);
  if (!this->Data->VisitCellsWithPoints.IndexOffsetsValid)
  {
    auto self = const_cast<Thisclass*>(this);
    auto functor =
      detail::BuildVisitCellsWithPointsConnectivityFunctor<VisitCellsWithPointsConnectivity>(
        self->Data->VisitCellsWithPoints);
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
                    vtkm::TopologyElementTagPoint,
                    vtkm::TopologyElementTagCell) const
{
  using VisitCellsWithPointsConnectivity =
    typename ConnectivityChooser<vtkm::TopologyElementTagCell,
                                 vtkm::TopologyElementTagPoint>::ConnectivityType;
  using VisitPointsWithCellsConnectivity =
    typename ConnectivityChooser<vtkm::TopologyElementTagPoint,
                                 vtkm::TopologyElementTagCell>::ConnectivityType;

  if (!this->Data->VisitPointsWithCells.ElementsValid ||
      !this->Data->VisitPointsWithCells.IndexOffsetsValid)
  {
    auto self = const_cast<Thisclass*>(this);
    auto functor =
      detail::BuildVisitPointsWithCellsConnectivityFunctor<VisitCellsWithPointsConnectivity,
                                                           VisitPointsWithCellsConnectivity>(
        self->Data->VisitCellsWithPoints,
        self->Data->VisitPointsWithCells,
        this->Data->NumberOfPoints);
    if (!vtkm::cont::TryExecuteOnDevice(device, functor))
    {
      throw vtkm::cont::ErrorExecution("Failed to run BuildConnectivity.");
    }
  }
}
}
} // vtkm::cont
#endif
