//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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
class ConnectivityPermutedPointToCell
{
public:
  using SchedulingRangeType = typename OriginalConnectivity::SchedulingRangeType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ConnectivityPermutedPointToCell()
    : Portal()
    , Connectivity()
  {
  }

  VTKM_EXEC_CONT
  ConnectivityPermutedPointToCell(const PermutationPortal& portal, const OriginalConnectivity& src)
    : Portal(portal)
    , Connectivity(src)
  {
  }

  VTKM_EXEC_CONT
  ConnectivityPermutedPointToCell(const ConnectivityPermutedPointToCell& src)
    : Portal(src.Portal)
    , Connectivity(src.Connectivity)
  {
  }

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

template <typename ConnectivityPortalType,
          typename NumIndicesPortalType,
          typename IndexOffsetPortalType>
class ConnectivityPermutedCellToPoint
{
public:
  using SchedulingRangeType = vtkm::Id;
  using IndicesType = vtkm::VecFromPortal<ConnectivityPortalType>;
  using CellShapeTag = vtkm::CellShapeTagVertex;

  ConnectivityPermutedCellToPoint() = default;

  ConnectivityPermutedCellToPoint(const ConnectivityPortalType& connectivity,
                                  const NumIndicesPortalType& numIndices,
                                  const IndexOffsetPortalType& indexOffset)
    : Connectivity(connectivity)
    , NumIndices(numIndices)
    , IndexOffset(indexOffset)
  {
  }

  VTKM_EXEC
  SchedulingRangeType GetNumberOfElements() const { return this->NumIndices.GetNumberOfValues(); }

  VTKM_EXEC CellShapeTag GetCellShape(vtkm::Id) const { return CellShapeTag(); }

  VTKM_EXEC
  vtkm::IdComponent GetNumberOfIndices(vtkm::Id index) const { return this->NumIndices.Get(index); }

  VTKM_EXEC IndicesType GetIndices(vtkm::Id index) const
  {
    return IndicesType(
      this->Connectivity, this->NumIndices.Get(index), this->IndexOffset.Get(index));
  }

private:
  ConnectivityPortalType Connectivity;
  NumIndicesPortalType NumIndices;
  IndexOffsetPortalType IndexOffset;
};
}
} // namespace vtkm::exec

#endif //vtk_m_exec_ConnectivityPermuted_h
