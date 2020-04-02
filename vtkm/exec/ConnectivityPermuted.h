//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_exec_ConnectivityPermuted_h
#define vtk_m_exec_ConnectivityPermuted_h

#include <vtkm/CellShape.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>
#include <vtkm/VecFromPortal.h>

namespace vtkm
{
namespace exec
{

template <typename PermutationPortal, typename OriginalConnectivity>
class ConnectivityPermutedVisitCellsWithPoints
{
public:
  using SchedulingRangeType = typename OriginalConnectivity::SchedulingRangeType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ConnectivityPermutedVisitCellsWithPoints()
    : Portal()
    , Connectivity()
  {
  }

  VTKM_EXEC_CONT
  ConnectivityPermutedVisitCellsWithPoints(const PermutationPortal& portal,
                                           const OriginalConnectivity& src)
    : Portal(portal)
    , Connectivity(src)
  {
  }

  VTKM_EXEC_CONT
  ConnectivityPermutedVisitCellsWithPoints(const ConnectivityPermutedVisitCellsWithPoints& src)
    : Portal(src.Portal)
    , Connectivity(src.Connectivity)
  {
  }

  ConnectivityPermutedVisitCellsWithPoints& operator=(
    const ConnectivityPermutedVisitCellsWithPoints& src) = default;
  ConnectivityPermutedVisitCellsWithPoints& operator=(
    ConnectivityPermutedVisitCellsWithPoints&& src) = default;

  VTKM_EXEC
  vtkm::Id GetNumberOfElements() const { return this->Portal.GetNumberOfValues(); }

  using CellShapeTag = typename OriginalConnectivity::CellShapeTag;

  VTKM_EXEC
  CellShapeTag GetCellShape(vtkm::Id index) const
  {
    vtkm::Id pIndex = this->Portal.Get(index);
    return this->Connectivity.GetCellShape(pIndex);
  }

  VTKM_EXEC
  vtkm::IdComponent GetNumberOfIndices(vtkm::Id index) const
  {
    return this->Connectivity.GetNumberOfIndices(this->Portal.Get(index));
  }

  using IndicesType = typename OriginalConnectivity::IndicesType;

  template <typename IndexType>
  VTKM_EXEC IndicesType GetIndices(const IndexType& index) const
  {
    return this->Connectivity.GetIndices(this->Portal.Get(index));
  }

  PermutationPortal Portal;
  OriginalConnectivity Connectivity;
};

template <typename ConnectivityPortalType, typename OffsetPortalType>
class ConnectivityPermutedVisitPointsWithCells
{
public:
  using SchedulingRangeType = vtkm::Id;
  using IndicesType = vtkm::VecFromPortal<ConnectivityPortalType>;
  using CellShapeTag = vtkm::CellShapeTagVertex;

  ConnectivityPermutedVisitPointsWithCells() = default;

  ConnectivityPermutedVisitPointsWithCells(const ConnectivityPortalType& connectivity,
                                           const OffsetPortalType& offsets)
    : Connectivity(connectivity)
    , Offsets(offsets)
  {
  }

  VTKM_EXEC
  SchedulingRangeType GetNumberOfElements() const { return this->Offsets.GetNumberOfValues() - 1; }

  VTKM_EXEC CellShapeTag GetCellShape(vtkm::Id) const { return CellShapeTag(); }

  VTKM_EXEC
  vtkm::IdComponent GetNumberOfIndices(vtkm::Id index) const
  {
    const vtkm::Id offBegin = this->Offsets.Get(index);
    const vtkm::Id offEnd = this->Offsets.Get(index + 1);
    return static_cast<vtkm::IdComponent>(offEnd - offBegin);
  }

  VTKM_EXEC IndicesType GetIndices(vtkm::Id index) const
  {
    const vtkm::Id offBegin = this->Offsets.Get(index);
    const vtkm::Id offEnd = this->Offsets.Get(index + 1);
    return IndicesType(
      this->Connectivity, static_cast<vtkm::IdComponent>(offEnd - offBegin), offBegin);
  }

private:
  ConnectivityPortalType Connectivity;
  OffsetPortalType Offsets;
};
}
} // namespace vtkm::exec

#endif //vtk_m_exec_ConnectivityPermuted_h
