//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_exec_ConnectivityStructured_h
#define vtk_m_exec_ConnectivityStructured_h

#include <vtkm/Deprecated.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>
#include <vtkm/internal/ConnectivityStructuredInternals.h>

namespace vtkm
{
namespace exec
{

template <typename VisitTopology, typename IncidentTopology, vtkm::IdComponent Dimension>
class ConnectivityStructured
{
  VTKM_IS_TOPOLOGY_ELEMENT_TAG(VisitTopology);
  VTKM_IS_TOPOLOGY_ELEMENT_TAG(IncidentTopology);

  using InternalsType = vtkm::internal::ConnectivityStructuredInternals<Dimension>;

  using Helper =
    vtkm::internal::ConnectivityStructuredIndexHelper<VisitTopology, IncidentTopology, Dimension>;

public:
  using SchedulingRangeType = typename InternalsType::SchedulingRangeType;

  ConnectivityStructured() = default;

  VTKM_EXEC_CONT
  ConnectivityStructured(const InternalsType& src)
    : Internals(src)
  {
  }

  ConnectivityStructured(const ConnectivityStructured& src) = default;

  VTKM_EXEC_CONT
  ConnectivityStructured(
    const ConnectivityStructured<IncidentTopology, VisitTopology, Dimension>& src)
    : Internals(src.Internals)
  {
  }


  ConnectivityStructured& operator=(const ConnectivityStructured& src) = default;
  ConnectivityStructured& operator=(ConnectivityStructured&& src) = default;


  VTKM_EXEC
  vtkm::Id GetNumberOfElements() const { return Helper::GetNumberOfElements(this->Internals); }

  using CellShapeTag = typename Helper::CellShapeTag;
  VTKM_EXEC
  CellShapeTag GetCellShape(vtkm::Id) const { return CellShapeTag(); }

  template <typename IndexType>
  VTKM_EXEC vtkm::IdComponent GetNumberOfIndices(const IndexType& index) const
  {
    return Helper::GetNumberOfIndices(this->Internals, index);
  }

  using IndicesType = typename Helper::IndicesType;

  template <typename IndexType>
  VTKM_EXEC IndicesType GetIndices(const IndexType& index) const
  {
    return Helper::GetIndices(this->Internals, index);
  }

  VTKM_EXEC_CONT SchedulingRangeType FlatToLogicalVisitIndex(vtkm::Id flatVisitIndex) const
  {
    return Helper::FlatToLogicalVisitIndex(this->Internals, flatVisitIndex);
  }

  VTKM_EXEC_CONT SchedulingRangeType FlatToLogicalIncidentIndex(vtkm::Id flatIncidentIndex) const
  {
    return Helper::FlatToLogicalIncidentIndex(this->Internals, flatIncidentIndex);
  }

  VTKM_EXEC_CONT vtkm::Id LogicalToFlatVisitIndex(
    const SchedulingRangeType& logicalVisitIndex) const
  {
    return Helper::LogicalToFlatVisitIndex(this->Internals, logicalVisitIndex);
  }

  VTKM_EXEC_CONT vtkm::Id LogicalToFlatIncidentIndex(
    const SchedulingRangeType& logicalIncidentIndex) const
  {
    return Helper::LogicalToFlatIncidentIndex(this->Internals, logicalIncidentIndex);
  }

  VTKM_EXEC_CONT
  VTKM_DEPRECATED(2.1, "Use FlatToLogicalIncidentIndex.")
  SchedulingRangeType FlatToLogicalFromIndex(vtkm::Id flatFromIndex) const
  {
    return this->FlatToLogicalIncidentIndex(flatFromIndex);
  }

  VTKM_EXEC_CONT
  VTKM_DEPRECATED(2.1, "Use LogicalToFlatIncidentIndex.")
  vtkm::Id LogicalToFlatFromIndex(const SchedulingRangeType& logicalFromIndex) const
  {
    return this->LogicalToFlatIncidentIndex(logicalFromIndex);
  }

  VTKM_EXEC_CONT
  VTKM_DEPRECATED(2.1, "Use FlatToLogicalVisitIndex.")
  SchedulingRangeType FlatToLogicalToIndex(vtkm::Id flatToIndex) const
  {
    return this->FlatToLogicalVisitIndex(flatToIndex);
  }

  VTKM_EXEC_CONT
  VTKM_DEPRECATED(2.1, "Use LogicalToFlatVisitIndex.")
  vtkm::Id LogicalToFlatToIndex(const SchedulingRangeType& logicalToIndex) const
  {
    return this->LogicalToFlatVisitIndex(logicalToIndex);
  }

  VTKM_EXEC_CONT
  vtkm::Vec<vtkm::Id, Dimension> GetPointDimensions() const
  {
    return this->Internals.GetPointDimensions();
  }

  VTKM_EXEC_CONT
  vtkm::Vec<vtkm::Id, Dimension> GetCellDimensions() const
  {
    return this->Internals.GetCellDimensions();
  }

  VTKM_EXEC_CONT
  SchedulingRangeType GetGlobalPointIndexStart() const
  {
    return this->Internals.GetGlobalPointIndexStart();
  }

  friend class ConnectivityStructured<IncidentTopology, VisitTopology, Dimension>;

private:
  InternalsType Internals;
};
}
} // namespace vtkm::exec

#endif //vtk_m_exec_ConnectivityStructured_h
