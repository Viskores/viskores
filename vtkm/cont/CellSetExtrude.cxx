//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/CellSetExtrude.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/CellShape.h>

namespace vtkm
{
namespace cont
{

CellSetExtrude::CellSetExtrude()
  : vtkm::cont::CellSet()
  , IsPeriodic(false)
  , NumberOfPointsPerPlane(0)
  , NumberOfCellsPerPlane(0)
  , NumberOfPlanes(0)
  , ReverseConnectivityBuilt(false)
{
}

CellSetExtrude::CellSetExtrude(const vtkm::cont::ArrayHandle<vtkm::Int32>& conn,
                               vtkm::Int32 numberOfPointsPerPlane,
                               vtkm::Int32 numberOfPlanes,
                               const vtkm::cont::ArrayHandle<vtkm::Int32>& nextNode,
                               bool periodic)
  : vtkm::cont::CellSet()
  , IsPeriodic(periodic)
  , NumberOfPointsPerPlane(numberOfPointsPerPlane)
  , NumberOfCellsPerPlane(
      static_cast<vtkm::Int32>(conn.GetNumberOfValues() / static_cast<vtkm::Id>(3)))
  , NumberOfPlanes(numberOfPlanes)
  , Connectivity(conn)
  , NextNode(nextNode)
  , ReverseConnectivityBuilt(false)
{
}


CellSetExtrude::CellSetExtrude(const CellSetExtrude& src)
  : CellSet(src)
  , IsPeriodic(src.IsPeriodic)
  , NumberOfPointsPerPlane(src.NumberOfPointsPerPlane)
  , NumberOfCellsPerPlane(src.NumberOfCellsPerPlane)
  , NumberOfPlanes(src.NumberOfPlanes)
  , Connectivity(src.Connectivity)
  , NextNode(src.NextNode)
  , ReverseConnectivityBuilt(src.ReverseConnectivityBuilt)
  , RConnectivity(src.RConnectivity)
  , ROffsets(src.ROffsets)
  , RCounts(src.RCounts)
  , PrevNode(src.PrevNode)
{
}

CellSetExtrude::CellSetExtrude(CellSetExtrude&& src) noexcept
  : CellSet(std::forward<CellSet>(src))
  , IsPeriodic(src.IsPeriodic)
  , NumberOfPointsPerPlane(src.NumberOfPointsPerPlane)
  , NumberOfCellsPerPlane(src.NumberOfCellsPerPlane)
  , NumberOfPlanes(src.NumberOfPlanes)
  , Connectivity(std::move(src.Connectivity))
  , NextNode(std::move(src.NextNode))
  , ReverseConnectivityBuilt(src.ReverseConnectivityBuilt)
  , RConnectivity(std::move(src.RConnectivity))
  , ROffsets(std::move(src.ROffsets))
  , RCounts(std::move(src.RCounts))
  , PrevNode(std::move(src.PrevNode))
{
}

CellSetExtrude& CellSetExtrude::operator=(const CellSetExtrude& src)
{
  this->CellSet::operator=(src);

  this->IsPeriodic = src.IsPeriodic;
  this->NumberOfPointsPerPlane = src.NumberOfPointsPerPlane;
  this->NumberOfCellsPerPlane = src.NumberOfCellsPerPlane;
  this->NumberOfPlanes = src.NumberOfPlanes;
  this->Connectivity = src.Connectivity;
  this->NextNode = src.NextNode;
  this->ReverseConnectivityBuilt = src.ReverseConnectivityBuilt;
  this->RConnectivity = src.RConnectivity;
  this->ROffsets = src.ROffsets;
  this->RCounts = src.RCounts;
  this->PrevNode = src.PrevNode;

  return *this;
}

CellSetExtrude& CellSetExtrude::operator=(CellSetExtrude&& src) noexcept
{
  this->CellSet::operator=(std::forward<CellSet>(src));

  this->IsPeriodic = src.IsPeriodic;
  this->NumberOfPointsPerPlane = src.NumberOfPointsPerPlane;
  this->NumberOfCellsPerPlane = src.NumberOfCellsPerPlane;
  this->NumberOfPlanes = src.NumberOfPlanes;
  this->Connectivity = std::move(src.Connectivity);
  this->NextNode = std::move(src.NextNode);
  this->ReverseConnectivityBuilt = src.ReverseConnectivityBuilt;
  this->RConnectivity = std::move(src.RConnectivity);
  this->ROffsets = std::move(src.ROffsets);
  this->RCounts = std::move(src.RCounts);
  this->PrevNode = std::move(src.PrevNode);

  return *this;
}

CellSetExtrude::~CellSetExtrude() {}

vtkm::Int32 CellSetExtrude::GetNumberOfPlanes() const
{
  return this->NumberOfPlanes;
}

vtkm::Id CellSetExtrude::GetNumberOfCells() const
{
  if (this->IsPeriodic)
  {
    return static_cast<vtkm::Id>(this->NumberOfPlanes) *
      static_cast<vtkm::Id>(this->NumberOfCellsPerPlane);
  }
  else
  {
    return static_cast<vtkm::Id>(this->NumberOfPlanes - 1) *
      static_cast<vtkm::Id>(this->NumberOfCellsPerPlane);
  }
}

vtkm::Id CellSetExtrude::GetNumberOfPoints() const
{
  return static_cast<vtkm::Id>(this->NumberOfPlanes) *
    static_cast<vtkm::Id>(this->NumberOfPointsPerPlane);
}

vtkm::Id CellSetExtrude::GetNumberOfFaces() const
{
  return -1;
}

vtkm::Id CellSetExtrude::GetNumberOfEdges() const
{
  return -1;
}

vtkm::UInt8 CellSetExtrude::GetCellShape(vtkm::Id) const
{
  return vtkm::CellShapeTagWedge::Id;
}

vtkm::IdComponent CellSetExtrude::GetNumberOfPointsInCell(vtkm::Id) const
{
  return 6;
}

void CellSetExtrude::GetCellPointIds(vtkm::Id id, vtkm::Id* ptids) const
{
  vtkm::cont::Token token;
  auto conn = this->PrepareForInput(vtkm::cont::DeviceAdapterTagSerial{},
                                    vtkm::TopologyElementTagCell{},
                                    vtkm::TopologyElementTagPoint{},
                                    token);
  auto indices = conn.GetIndices(id);
  for (int i = 0; i < 6; ++i)
  {
    ptids[i] = indices[i];
  }
}

template <vtkm::IdComponent NumIndices>
VTKM_CONT void CellSetExtrude::GetIndices(vtkm::Id index,
                                          vtkm::Vec<vtkm::Id, NumIndices>& ids) const
{
  static_assert(NumIndices == 6, "There are always 6 points in a wedge.");
  this->GetCellPointIds(index, ids.data());
}

VTKM_CONT void CellSetExtrude::GetIndices(vtkm::Id index,
                                          vtkm::cont::ArrayHandle<vtkm::Id>& ids) const
{
  ids.Allocate(6);
  auto outIdPortal = ids.WritePortal();
  vtkm::cont::Token token;
  auto conn = this->PrepareForInput(vtkm::cont::DeviceAdapterTagSerial{},
                                    vtkm::TopologyElementTagCell{},
                                    vtkm::TopologyElementTagPoint{},
                                    token);
  auto indices = conn.GetIndices(index);
  for (vtkm::IdComponent i = 0; i < 6; i++)
  {
    outIdPortal.Set(i, indices[i]);
  }
}

std::shared_ptr<CellSet> CellSetExtrude::NewInstance() const
{
  return std::make_shared<CellSetExtrude>();
}

void CellSetExtrude::DeepCopy(const CellSet* src)
{
  const auto* other = dynamic_cast<const CellSetExtrude*>(src);
  if (!other)
  {
    throw vtkm::cont::ErrorBadType("CellSetExplicit::DeepCopy types don't match");
  }

  this->IsPeriodic = other->IsPeriodic;

  this->NumberOfPointsPerPlane = other->NumberOfPointsPerPlane;
  this->NumberOfCellsPerPlane = other->NumberOfCellsPerPlane;
  this->NumberOfPlanes = other->NumberOfPlanes;

  vtkm::cont::ArrayCopy(other->Connectivity, this->Connectivity);
  vtkm::cont::ArrayCopy(other->NextNode, this->NextNode);

  this->ReverseConnectivityBuilt = other->ReverseConnectivityBuilt;

  if (this->ReverseConnectivityBuilt)
  {
    vtkm::cont::ArrayCopy(other->RConnectivity, this->RConnectivity);
    vtkm::cont::ArrayCopy(other->ROffsets, this->ROffsets);
    vtkm::cont::ArrayCopy(other->RCounts, this->RCounts);
    vtkm::cont::ArrayCopy(other->PrevNode, this->PrevNode);
  }
}

void CellSetExtrude::ReleaseResourcesExecution()
{
  this->Connectivity.ReleaseResourcesExecution();
  this->NextNode.ReleaseResourcesExecution();

  this->RConnectivity.ReleaseResourcesExecution();
  this->ROffsets.ReleaseResourcesExecution();
  this->RCounts.ReleaseResourcesExecution();
  this->PrevNode.ReleaseResourcesExecution();
}

vtkm::Id2 CellSetExtrude::GetSchedulingRange(vtkm::TopologyElementTagCell) const
{
  if (this->IsPeriodic)
  {
    return vtkm::Id2(this->NumberOfCellsPerPlane, this->NumberOfPlanes);
  }
  else
  {
    return vtkm::Id2(this->NumberOfCellsPerPlane, this->NumberOfPlanes - 1);
  }
}

vtkm::Id2 CellSetExtrude::GetSchedulingRange(vtkm::TopologyElementTagPoint) const
{
  return vtkm::Id2(this->NumberOfPointsPerPlane, this->NumberOfPlanes);
}

void CellSetExtrude::PrintSummary(std::ostream& out) const
{
  out << "   vtkmCellSetSingleType: " << std::endl;
  out << "   NumberOfCellsPerPlane: " << this->NumberOfCellsPerPlane << std::endl;
  out << "   NumberOfPointsPerPlane: " << this->NumberOfPointsPerPlane << std::endl;
  out << "   NumberOfPlanes: " << this->NumberOfPlanes << std::endl;
  out << "   Connectivity: " << std::endl;
  vtkm::cont::printSummary_ArrayHandle(this->Connectivity, out);
  out << "   NextNode: " << std::endl;
  vtkm::cont::printSummary_ArrayHandle(this->NextNode, out);
  out << "   ReverseConnectivityBuilt: " << this->NumberOfPlanes << std::endl;
}


namespace
{

struct ComputeReverseMapping : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn cellIndex, WholeArrayOut cellIds);

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename PortalType>
  VTKM_EXEC void operator()(vtkm::Id cellId, PortalType&& pointIdValue) const
  {
    //3 as we are building the connectivity for triangles
    const vtkm::Id offset = 3 * cellId;
    pointIdValue.Set(offset, static_cast<vtkm::Int32>(cellId));
    pointIdValue.Set(offset + 1, static_cast<vtkm::Int32>(cellId));
    pointIdValue.Set(offset + 2, static_cast<vtkm::Int32>(cellId));
  }
};

struct ComputePrevNode : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn nextNode, WholeArrayOut prevNodeArray);
  typedef void ExecutionSignature(InputIndex, _1, _2);

  template <typename PortalType>
  VTKM_EXEC void operator()(vtkm::Id idx, vtkm::Int32 next, PortalType& prevs) const
  {
    prevs.Set(static_cast<vtkm::Id>(next), static_cast<vtkm::Int32>(idx));
  }
};

} // anonymous namespace

VTKM_CONT void CellSetExtrude::BuildReverseConnectivity()
{
  vtkm::cont::Invoker invoke;

  // create a mapping of where each key is the point id and the value
  // is the cell id. We
  const vtkm::Id numberOfPointsPerCell = 3;
  const vtkm::Id rconnSize = this->NumberOfCellsPerPlane * numberOfPointsPerCell;

  vtkm::cont::ArrayHandle<vtkm::Int32> pointIdKey;
  vtkm::cont::ArrayCopy(this->Connectivity, pointIdKey);

  this->RConnectivity.Allocate(rconnSize);
  invoke(ComputeReverseMapping{},
         vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, 1, this->NumberOfCellsPerPlane),
         this->RConnectivity);

  vtkm::cont::Algorithm::SortByKey(pointIdKey, this->RConnectivity);

  // now we can compute the counts and offsets
  vtkm::cont::ArrayHandle<vtkm::Int32> reducedKeys;
  vtkm::cont::Algorithm::ReduceByKey(
    pointIdKey,
    vtkm::cont::make_ArrayHandleConstant(vtkm::Int32(1), static_cast<vtkm::Int32>(rconnSize)),
    reducedKeys,
    this->RCounts,
    vtkm::Add{});

  vtkm::cont::Algorithm::ScanExclusive(this->RCounts, this->ROffsets);

  // compute PrevNode from NextNode
  this->PrevNode.Allocate(this->NextNode.GetNumberOfValues());
  invoke(ComputePrevNode{}, this->NextNode, this->PrevNode);

  this->ReverseConnectivityBuilt = true;
}

vtkm::exec::ConnectivityExtrude CellSetExtrude::PrepareForInput(vtkm::cont::DeviceAdapterId device,
                                                                vtkm::TopologyElementTagCell,
                                                                vtkm::TopologyElementTagPoint,
                                                                vtkm::cont::Token& token) const
{
  return vtkm::exec::ConnectivityExtrude(this->Connectivity.PrepareForInput(device, token),
                                         this->NextNode.PrepareForInput(device, token),
                                         this->NumberOfCellsPerPlane,
                                         this->NumberOfPointsPerPlane,
                                         this->NumberOfPlanes,
                                         this->IsPeriodic);
}

vtkm::exec::ReverseConnectivityExtrude CellSetExtrude::PrepareForInput(
  vtkm::cont::DeviceAdapterId device,
  vtkm::TopologyElementTagPoint,
  vtkm::TopologyElementTagCell,
  vtkm::cont::Token& token) const
{
  if (!this->ReverseConnectivityBuilt)
  {
    vtkm::cont::ScopedRuntimeDeviceTracker tracker(device);
    const_cast<CellSetExtrude*>(this)->BuildReverseConnectivity();
  }
  return vtkm::exec::ReverseConnectivityExtrude(this->RConnectivity.PrepareForInput(device, token),
                                                this->ROffsets.PrepareForInput(device, token),
                                                this->RCounts.PrepareForInput(device, token),
                                                this->PrevNode.PrepareForInput(device, token),
                                                this->NumberOfCellsPerPlane,
                                                this->NumberOfPointsPerPlane,
                                                this->NumberOfPlanes);
}

}
} // vtkm::cont
