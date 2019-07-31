//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_ConnectivityExtrude_h
#define vtk_m_exec_ConnectivityExtrude_h

#include <vtkm/internal/IndicesExtrude.h>

#include <vtkm/CellShape.h>
#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/exec/arg/ThreadIndicesTopologyMap.h>


namespace vtkm
{
namespace exec
{

template <typename Device>
class VTKM_ALWAYS_EXPORT ConnectivityExtrude
{
private:
  using Int32HandleType = vtkm::cont::ArrayHandle<vtkm::Int32>;
  using Int32PortalType = typename Int32HandleType::template ExecutionTypes<Device>::PortalConst;

public:
  using ConnectivityPortalType = Int32PortalType;
  using NextNodePortalType = Int32PortalType;

  using SchedulingRangeType = vtkm::Id2;

  using CellShapeTag = vtkm::CellShapeTagWedge;

  using IndicesType = IndicesExtrude;

  ConnectivityExtrude() = default;

  ConnectivityExtrude(const ConnectivityPortalType& conn,
                      const NextNodePortalType& nextnode,
                      vtkm::Int32 cellsPerPlane,
                      vtkm::Int32 pointsPerPlane,
                      vtkm::Int32 numPlanes,
                      bool periodic);

  VTKM_EXEC
  vtkm::Id GetNumberOfElements() const { return this->NumberOfCells; }

  VTKM_EXEC
  CellShapeTag GetCellShape(vtkm::Id) const { return vtkm::CellShapeTagWedge(); }

  VTKM_EXEC
  IndicesType GetIndices(vtkm::Id index) const
  {
    return this->GetIndices(this->FlatToLogicalToIndex(index));
  }

  VTKM_EXEC
  IndicesType GetIndices(const vtkm::Id2& index) const;
  template <typename IndexType>
  VTKM_EXEC vtkm::IdComponent GetNumberOfIndices(const IndexType& vtkmNotUsed(index)) const
  {
    return 6;
  }

  VTKM_EXEC
  vtkm::Id LogicalToFlatToIndex(const vtkm::Id2& index) const
  {
    return index[0] + (index[1] * this->NumberOfCellsPerPlane);
  };

  VTKM_EXEC
  vtkm::Id2 FlatToLogicalToIndex(vtkm::Id index) const
  {
    const vtkm::Id cellId = index % this->NumberOfCellsPerPlane;
    const vtkm::Id plane = index / this->NumberOfCellsPerPlane;
    return vtkm::Id2(cellId, plane);
  }

private:
  ConnectivityPortalType Connectivity;
  NextNodePortalType NextNode;
  vtkm::Int32 NumberOfCellsPerPlane;
  vtkm::Int32 NumberOfPointsPerPlane;
  vtkm::Int32 NumberOfPlanes;
  vtkm::Id NumberOfCells;
};


template <typename Device>
class ReverseConnectivityExtrude
{
private:
  using Int32HandleType = vtkm::cont::ArrayHandle<vtkm::Int32>;
  using Int32PortalType = typename Int32HandleType::template ExecutionTypes<Device>::PortalConst;

public:
  using ConnectivityPortalType = Int32PortalType;
  using OffsetsPortalType = Int32PortalType;
  using CountsPortalType = Int32PortalType;
  using PrevNodePortalType = Int32PortalType;

  using SchedulingRangeType = vtkm::Id2;

  using CellShapeTag = vtkm::CellShapeTagVertex;

  using IndicesType = ReverseIndicesExtrude<ConnectivityPortalType>;

  ReverseConnectivityExtrude() = default;

  VTKM_EXEC
  ReverseConnectivityExtrude(const ConnectivityPortalType& conn,
                             const OffsetsPortalType& offsets,
                             const CountsPortalType& counts,
                             const PrevNodePortalType& prevNode,
                             vtkm::Int32 cellsPerPlane,
                             vtkm::Int32 pointsPerPlane,
                             vtkm::Int32 numPlanes);

  VTKM_EXEC
  vtkm::Id GetNumberOfElements() const
  {
    return this->NumberOfPointsPerPlane * this->NumberOfPlanes;
  }

  VTKM_EXEC
  CellShapeTag GetCellShape(vtkm::Id) const { return vtkm::CellShapeTagVertex(); }

  /// Returns a Vec-like object containing the indices for the given index.
  /// The object returned is not an actual array, but rather an object that
  /// loads the indices lazily out of the connectivity array. This prevents
  /// us from having to know the number of indices at compile time.
  ///
  VTKM_EXEC
  IndicesType GetIndices(vtkm::Id index) const
  {
    return this->GetIndices(this->FlatToLogicalToIndex(index));
  }

  VTKM_EXEC
  IndicesType GetIndices(const vtkm::Id2& index) const;

  template <typename IndexType>
  VTKM_EXEC vtkm::IdComponent GetNumberOfIndices(const IndexType& vtkmNotUsed(index)) const
  {
    return 1;
  }

  VTKM_EXEC
  vtkm::Id LogicalToFlatToIndex(const vtkm::Id2& index) const
  {
    return index[0] + (index[1] * this->NumberOfPointsPerPlane);
  };

  VTKM_EXEC
  vtkm::Id2 FlatToLogicalToIndex(vtkm::Id index) const
  {
    const vtkm::Id vertId = index % this->NumberOfPointsPerPlane;
    const vtkm::Id plane = index / this->NumberOfPointsPerPlane;
    return vtkm::Id2(vertId, plane);
  }

  ConnectivityPortalType Connectivity;
  OffsetsPortalType Offsets;
  CountsPortalType Counts;
  PrevNodePortalType PrevNode;
  vtkm::Int32 NumberOfCellsPerPlane;
  vtkm::Int32 NumberOfPointsPerPlane;
  vtkm::Int32 NumberOfPlanes;
};


template <typename Device>
ConnectivityExtrude<Device>::ConnectivityExtrude(const ConnectivityPortalType& conn,
                                                 const ConnectivityPortalType& nextNode,
                                                 vtkm::Int32 cellsPerPlane,
                                                 vtkm::Int32 pointsPerPlane,
                                                 vtkm::Int32 numPlanes,
                                                 bool periodic)
  : Connectivity(conn)
  , NextNode(nextNode)
  , NumberOfCellsPerPlane(cellsPerPlane)
  , NumberOfPointsPerPlane(pointsPerPlane)
  , NumberOfPlanes(numPlanes)
{
  this->NumberOfCells = periodic ? (static_cast<vtkm::Id>(cellsPerPlane) * numPlanes)
                                 : (static_cast<vtkm::Id>(cellsPerPlane) * (numPlanes - 1));
}

template <typename Device>
typename ConnectivityExtrude<Device>::IndicesType ConnectivityExtrude<Device>::GetIndices(
  const vtkm::Id2& index) const
{
  vtkm::Id tr = index[0];
  vtkm::Id p0 = index[1];
  vtkm::Id p1 = (p0 < (this->NumberOfPlanes - 1)) ? (p0 + 1) : 0;

  vtkm::Vec3i_32 pointIds1, pointIds2;
  for (int i = 0; i < 3; ++i)
  {
    pointIds1[i] = this->Connectivity.Get((tr * 3) + i);
    pointIds2[i] = this->NextNode.Get(pointIds1[i]);
  }

  return IndicesType(pointIds1,
                     static_cast<vtkm::Int32>(p0),
                     pointIds2,
                     static_cast<vtkm::Int32>(p1),
                     this->NumberOfPointsPerPlane);
}


template <typename Device>
ReverseConnectivityExtrude<Device>::ReverseConnectivityExtrude(const ConnectivityPortalType& conn,
                                                               const OffsetsPortalType& offsets,
                                                               const CountsPortalType& counts,
                                                               const PrevNodePortalType& prevNode,
                                                               vtkm::Int32 cellsPerPlane,
                                                               vtkm::Int32 pointsPerPlane,
                                                               vtkm::Int32 numPlanes)
  : Connectivity(conn)
  , Offsets(offsets)
  , Counts(counts)
  , PrevNode(prevNode)
  , NumberOfCellsPerPlane(cellsPerPlane)
  , NumberOfPointsPerPlane(pointsPerPlane)
  , NumberOfPlanes(numPlanes)
{
}

template <typename Device>
typename ReverseConnectivityExtrude<Device>::IndicesType
ReverseConnectivityExtrude<Device>::GetIndices(const vtkm::Id2& index) const
{
  auto ptCur = index[0];
  auto ptPre = this->PrevNode.Get(ptCur);
  auto plCur = index[1];
  auto plPre = (plCur == 0) ? (this->NumberOfPlanes - 1) : (plCur - 1);

  return IndicesType(this->Connectivity,
                     this->Offsets.Get(ptPre),
                     this->Counts.Get(ptPre),
                     this->Offsets.Get(ptCur),
                     this->Counts.Get(ptCur),
                     static_cast<vtkm::IdComponent>(plPre),
                     static_cast<vtkm::IdComponent>(plCur),
                     this->NumberOfCellsPerPlane);
}
}
} // namespace vtkm::exec
#endif
