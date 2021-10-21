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
#include <vtkm/cont/CellSetExplicit.h>

#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/Logging.h>

// This file uses a lot of very verbose identifiers and the clang formatted
// code quickly becomes unreadable. Stick with manual formatting for now.
//
// clang-format off

namespace vtkm
{
namespace cont
{

template <typename SST, typename CST, typename OST>
VTKM_CONT
CellSetExplicit<SST, CST, OST>::CellSetExplicit()
  : CellSet()
  , Data(std::make_shared<Internals>())
{
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
CellSetExplicit<SST, CST, OST>::CellSetExplicit(const Thisclass& src)
  : CellSet(src)
  , Data(src.Data)
{
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
CellSetExplicit<SST, CST, OST>::CellSetExplicit(Thisclass &&src) noexcept
  : CellSet(std::forward<CellSet>(src))
  , Data(std::move(src.Data))
{
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
auto CellSetExplicit<SST, CST, OST>::operator=(const Thisclass& src)
-> Thisclass&
{
  this->CellSet::operator=(src);
  this->Data = src.Data;
  return *this;
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
auto CellSetExplicit<SST, CST, OST>::operator=(Thisclass&& src) noexcept
-> Thisclass&
{
  this->CellSet::operator=(std::forward<CellSet>(src));
  this->Data = std::move(src.Data);
  return *this;
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
CellSetExplicit<SST, CST, OST>::~CellSetExplicit()
{
  // explicitly define instead of '=default' to workaround an intel compiler bug
  // (see #179)
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
void CellSetExplicit<SST, CST, OST>::PrintSummary(std::ostream& out) const
{
  out << "   ExplicitCellSet:" << std::endl;
  out << "   CellPointIds:" << std::endl;
  this->Data->CellPointIds.PrintSummary(out);
  out << "   PointCellIds:" << std::endl;
  this->Data->PointCellIds.PrintSummary(out);
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
void CellSetExplicit<SST, CST, OST>::ReleaseResourcesExecution()
{
  this->Data->CellPointIds.ReleaseResourcesExecution();
  this->Data->PointCellIds.ReleaseResourcesExecution();
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
vtkm::Id CellSetExplicit<SST, CST, OST>::GetNumberOfCells() const
{
  return this->Data->CellPointIds.GetNumberOfElements();
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
vtkm::Id CellSetExplicit<SST, CST, OST>::GetNumberOfPoints() const
{
  return this->Data->NumberOfPoints;
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
vtkm::Id CellSetExplicit<SST, CST, OST>::GetNumberOfFaces() const
{
  return -1;
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
vtkm::Id CellSetExplicit<SST, CST, OST>::GetNumberOfEdges() const
{
  return -1;
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
VTKM_CONT
void CellSetExplicit<SST, CST, OST>::GetCellPointIds(vtkm::Id cellId,
                                                     vtkm::Id* ptids) const
{
  const auto offPortal = this->Data->CellPointIds.Offsets.ReadPortal();
  const vtkm::Id start = offPortal.Get(cellId);
  const vtkm::Id end = offPortal.Get(cellId + 1);
  const vtkm::IdComponent numIndices = static_cast<vtkm::IdComponent>(end - start);
  auto connPortal = this->Data->CellPointIds.Connectivity.ReadPortal();
  for (vtkm::IdComponent i = 0; i < numIndices; i++)
  {
    ptids[i] = connPortal.Get(start + i);
  }
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
VTKM_CONT
vtkm::Id CellSetExplicit<SST, CST, OST>
::GetSchedulingRange(vtkm::TopologyElementTagCell) const
{
  return this->GetNumberOfCells();
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
vtkm::Id CellSetExplicit<SST, CST, OST>
::GetSchedulingRange(vtkm::TopologyElementTagPoint) const
{
  return this->GetNumberOfPoints();
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
vtkm::IdComponent CellSetExplicit<SST, CST, OST>
::GetNumberOfPointsInCell(vtkm::Id cellid) const
{
  const auto portal = this->Data->CellPointIds.Offsets.ReadPortal();
  return static_cast<vtkm::IdComponent>(portal.Get(cellid + 1) -
                                        portal.Get(cellid));
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
typename vtkm::cont::ArrayHandle<vtkm::UInt8, SST>::ReadPortalType
CellSetExplicit<SST, CST, OST>::ShapesReadPortal() const
{
  return this->Data->CellPointIds.Shapes.ReadPortal();
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
VTKM_DEPRECATED(1.6, "Calling GetCellShape(cellid) is a performance bug. Call ShapesReadPortal() and loop over the Get.")
vtkm::UInt8 CellSetExplicit<SST, CST, OST>
::GetCellShape(vtkm::Id cellid) const
{
  return this->ShapesReadPortal().Get(cellid);
}


template <typename SST, typename CST, typename OST>
template <vtkm::IdComponent NumVecIndices>
VTKM_CONT
void CellSetExplicit<SST, CST, OST>
::GetIndices(vtkm::Id cellId, vtkm::Vec<vtkm::Id, NumVecIndices>& ids) const
{
  const auto offPortal = this->Data->CellPointIds.Offsets.ReadPortal();
  const vtkm::Id start = offPortal.Get(cellId);
  const vtkm::Id end = offPortal.Get(cellId + 1);
  const auto numCellIndices = static_cast<vtkm::IdComponent>(end - start);
  const auto connPortal = this->Data->CellPointIds.Connectivity.ReadPortal();

  VTKM_LOG_IF_S(vtkm::cont::LogLevel::Warn,
                numCellIndices != NumVecIndices,
                "GetIndices given a " << NumVecIndices
                << "-vec to fetch a cell with " << numCellIndices << "points. "
                "Truncating result.");

  const vtkm::IdComponent numIndices = vtkm::Min(NumVecIndices, numCellIndices);

  for (vtkm::IdComponent i = 0; i < numIndices; i++)
  {
    ids[i] = connPortal.Get(start + i);
  }
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
void CellSetExplicit<SST, CST, OST>
::GetIndices(vtkm::Id cellId, vtkm::cont::ArrayHandle<vtkm::Id>& ids) const
{
  const auto offPortal = this->Data->CellPointIds.Offsets.ReadPortal();
  const vtkm::Id start = offPortal.Get(cellId);
  const vtkm::Id end = offPortal.Get(cellId + 1);
  const vtkm::IdComponent numIndices = static_cast<vtkm::IdComponent>(end - start);
  ids.Allocate(numIndices);
  auto connPortal = this->Data->CellPointIds.Connectivity.ReadPortal();

  auto outIdPortal = ids.WritePortal();

  for (vtkm::IdComponent i = 0; i < numIndices; i++)
  {
    outIdPortal.Set(i, connPortal.Get(start + i));
  }
}


//----------------------------------------------------------------------------
namespace internal
{

// Sets the first value of the array to zero if the handle is writable,
// otherwise do nothing:
template <typename ArrayType>
typename std::enable_if<vtkm::cont::internal::IsWritableArrayHandle<ArrayType>::value>::type
SetFirstToZeroIfWritable(ArrayType&& array)
{
  using ValueType = typename std::decay<ArrayType>::type::ValueType;
  using Traits = vtkm::TypeTraits<ValueType>;
  array.WritePortal().Set(0, Traits::ZeroInitialization());
}

template <typename ArrayType>
typename std::enable_if<!vtkm::cont::internal::IsWritableArrayHandle<ArrayType>::value>::type
SetFirstToZeroIfWritable(ArrayType&&)
{ /* no-op */ }

} // end namespace internal

template <typename SST, typename CST, typename OST>
VTKM_CONT
void CellSetExplicit<SST, CST, OST>
::PrepareToAddCells(vtkm::Id numCells,
                    vtkm::Id connectivityMaxLen)
{
  this->Data->CellPointIds.Shapes.Allocate(numCells);
  this->Data->CellPointIds.Connectivity.Allocate(connectivityMaxLen);
  this->Data->CellPointIds.Offsets.Allocate(numCells + 1);
  internal::SetFirstToZeroIfWritable(this->Data->CellPointIds.Offsets);
  this->Data->NumberOfCellsAdded = 0;
  this->Data->ConnectivityAdded = 0;
}

template <typename SST, typename CST, typename OST>
template <typename IdVecType>
VTKM_CONT
void CellSetExplicit<SST, CST, OST>::AddCell(vtkm::UInt8 cellType,
                                             vtkm::IdComponent numVertices,
                                             const IdVecType& ids)
{
  using Traits = vtkm::VecTraits<IdVecType>;
  VTKM_STATIC_ASSERT_MSG((std::is_same<typename Traits::ComponentType, vtkm::Id>::value),
                         "CellSetSingleType::AddCell requires vtkm::Id for indices.");

  if (Traits::GetNumberOfComponents(ids) < numVertices)
  {
    throw vtkm::cont::ErrorBadValue("Not enough indices given to CellSetExplicit::AddCell.");
  }

  if (this->Data->NumberOfCellsAdded >= this->Data->CellPointIds.Shapes.GetNumberOfValues())
  {
    throw vtkm::cont::ErrorBadValue("Added more cells then expected.");
  }
  if (this->Data->ConnectivityAdded + numVertices >
      this->Data->CellPointIds.Connectivity.GetNumberOfValues())
  {
    throw vtkm::cont::ErrorBadValue(
      "Connectivity increased past estimated maximum connectivity.");
  }

  auto shapes = this->Data->CellPointIds.Shapes.WritePortal();
  auto conn = this->Data->CellPointIds.Connectivity.WritePortal();
  auto offsets = this->Data->CellPointIds.Offsets.WritePortal();

  shapes.Set(this->Data->NumberOfCellsAdded, cellType);
  for (vtkm::IdComponent iVec = 0; iVec < numVertices; ++iVec)
  {
    conn.Set(this->Data->ConnectivityAdded + iVec,
             Traits::GetComponent(ids, iVec));
  }

  this->Data->NumberOfCellsAdded++;
  this->Data->ConnectivityAdded += numVertices;

  // Set the end offset for the added cell:
  offsets.Set(this->Data->NumberOfCellsAdded, this->Data->ConnectivityAdded);
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
void CellSetExplicit<SST, CST, OST>::CompleteAddingCells(vtkm::Id numPoints)
{
  this->Data->NumberOfPoints = numPoints;
  this->Data->CellPointIds.Connectivity.Allocate(this->Data->ConnectivityAdded, vtkm::CopyFlag::On);
  this->Data->CellPointIds.ElementsValid = true;

  if (this->Data->NumberOfCellsAdded != this->GetNumberOfCells())
  {
    throw vtkm::cont::ErrorBadValue("Did not add as many cells as expected.");
  }

  this->Data->NumberOfCellsAdded = -1;
  this->Data->ConnectivityAdded = -1;
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
VTKM_CONT
void CellSetExplicit<SST, CST, OST>
::Fill(vtkm::Id numPoints,
       const vtkm::cont::ArrayHandle<vtkm::UInt8, SST>& shapes,
       const vtkm::cont::ArrayHandle<vtkm::Id, CST>& connectivity,
       const vtkm::cont::ArrayHandle<vtkm::Id, OST>& offsets)
{
  // Validate inputs:
  // Even for an empty cellset, offsets must contain a single 0:
  VTKM_ASSERT(offsets.GetNumberOfValues() > 0);
  // Must be [numCells + 1] offsets and [numCells] shapes
  VTKM_ASSERT(offsets.GetNumberOfValues() == shapes.GetNumberOfValues() + 1);
  // The last offset must be the size of the connectivity array.
  VTKM_ASSERT(vtkm::cont::ArrayGetValue(offsets.GetNumberOfValues() - 1,
                                        offsets) ==
              connectivity.GetNumberOfValues());

  this->Data->NumberOfPoints = numPoints;
  this->Data->CellPointIds.Shapes = shapes;
  this->Data->CellPointIds.Connectivity = connectivity;
  this->Data->CellPointIds.Offsets = offsets;

  this->Data->CellPointIds.ElementsValid = true;

  this->ResetConnectivity(TopologyElementTagPoint{}, TopologyElementTagCell{});
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
VTKM_CONT
auto CellSetExplicit<SST, CST, OST>
::PrepareForInput(vtkm::cont::DeviceAdapterId device, VisitTopology, IncidentTopology, vtkm::cont::Token& token) const
-> ExecConnectivityType<VisitTopology, IncidentTopology>
{
  this->BuildConnectivity(device, VisitTopology{}, IncidentTopology{});

  const auto& connectivity = this->GetConnectivity(VisitTopology{},
                                                   IncidentTopology{});
  VTKM_ASSERT(connectivity.ElementsValid);

  using ExecObjType = ExecConnectivityType<VisitTopology, IncidentTopology>;

  return ExecObjType(connectivity.Shapes.PrepareForInput(device, token),
                     connectivity.Connectivity.PrepareForInput(device, token),
                     connectivity.Offsets.PrepareForInput(device, token));
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
VTKM_CONT auto CellSetExplicit<SST, CST, OST>
::GetShapesArray(VisitTopology, IncidentTopology) const
-> const typename ConnectivityChooser<VisitTopology,
                                      IncidentTopology>::ShapesArrayType&
{
  this->BuildConnectivity(vtkm::cont::DeviceAdapterTagAny{},
                          VisitTopology{},
                          IncidentTopology{});
  return this->GetConnectivity(VisitTopology{}, IncidentTopology{}).Shapes;
}

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
VTKM_CONT
auto CellSetExplicit<SST, CST, OST>
::GetConnectivityArray(VisitTopology, IncidentTopology) const
-> const typename ConnectivityChooser<VisitTopology,
                                      IncidentTopology>::ConnectivityArrayType&
{
  this->BuildConnectivity(vtkm::cont::DeviceAdapterTagAny{},
                          VisitTopology{},
                          IncidentTopology{});
  return this->GetConnectivity(VisitTopology{},
                               IncidentTopology{}).Connectivity;
}

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
VTKM_CONT
auto CellSetExplicit<SST, CST, OST>
::GetOffsetsArray(VisitTopology, IncidentTopology) const
-> const typename ConnectivityChooser<VisitTopology,
                                      IncidentTopology>::OffsetsArrayType&
{
  this->BuildConnectivity(vtkm::cont::DeviceAdapterTagAny{},
                          VisitTopology{},
                          IncidentTopology{});
  return this->GetConnectivity(VisitTopology{},
                               IncidentTopology{}).Offsets;
}

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
VTKM_CONT
auto CellSetExplicit<SST, CST, OST>
::GetNumIndicesArray(VisitTopology visited, IncidentTopology incident) const
-> typename ConnectivityChooser<VisitTopology,
                                IncidentTopology>::NumIndicesArrayType
{
  // Converts to NumIndicesArrayType (which is an ArrayHandleOffsetsToNumComponents)
  return this->GetOffsetsArray(visited, incident);
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
VTKM_CONT
std::shared_ptr<CellSet> CellSetExplicit<SST, CST, OST>::NewInstance() const
{
  return std::make_shared<CellSetExplicit>();
}

template <typename SST, typename CST, typename OST>
VTKM_CONT
void CellSetExplicit<SST, CST, OST>::DeepCopy(const CellSet* src)
{
  const auto* other = dynamic_cast<const CellSetExplicit*>(src);
  if (!other)
  {
    throw vtkm::cont::ErrorBadType("CellSetExplicit::DeepCopy types don't match");
  }

  ShapesArrayType shapes;
  ConnectivityArrayType conn;
  OffsetsArrayType offsets;

  const auto ct = vtkm::TopologyElementTagCell{};
  const auto pt = vtkm::TopologyElementTagPoint{};

  shapes.DeepCopyFrom(other->GetShapesArray(ct, pt));
  conn.DeepCopyFrom(other->GetConnectivityArray(ct, pt));
  offsets.DeepCopyFrom(other->GetOffsetsArray(ct, pt));

  this->Fill(other->GetNumberOfPoints(), shapes, conn, offsets);
}

}
} // vtkm::cont

// clang-format on

#endif
