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

/// @brief A class holding information about topology connections.
///
/// An object of `ConnectivityStructured` is provided to a worklet when the
/// `ControlSignature` argument is `WholeCellSetIn` and the `vtkm::cont::CellSet`
/// provided is a `vtkm::cont::CellSetStructured`.
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


  /// @brief Provides the number of elements in the topology.
  ///
  /// This number of elements is associated with the "visit" type of topology element,
  /// which is the first template argument to `WholeCellSetIn`. The number of elements
  /// defines the valid indices for the other methods of this class.
  VTKM_EXEC
  vtkm::Id GetNumberOfElements() const { return Helper::GetNumberOfElements(this->Internals); }

  /// @brief The tag representing the cell shape of the visited elements.
  ///
  /// If the "visit" element is cells, then the returned tag is `vtkm::CellShapeTagHexahedron`
  /// for a 3D structured grid, `vtkm::CellShapeTagQuad` for a 2D structured grid, or
  /// `vtkm::CellShapeLine` for a 1D structured grid.
  using CellShapeTag = typename Helper::CellShapeTag;

  /// @brief Returns a tag for the cell shape associated with the element at the given index.
  ///
  /// If the "visit" element is cells, then the returned tag is `vtkm::CellShapeTagHexahedron`
  /// for a 3D structured grid, `vtkm::CellShapeTagQuad` for a 2D structured grid, or
  /// `vtkm::CellShapeLine` for a 1D structured grid.
  VTKM_EXEC
  CellShapeTag GetCellShape(vtkm::Id) const { return CellShapeTag(); }

  /// Given the index of a visited element, returns the number of incident elements
  /// touching it.
  template <typename IndexType>
  VTKM_EXEC vtkm::IdComponent GetNumberOfIndices(const IndexType& index) const
  {
    return Helper::GetNumberOfIndices(this->Internals, index);
  }

  /// @brief Type of variable that lists of incident indices will be put into.
  using IndicesType = typename Helper::IndicesType;

  /// Provides the indices of all elements incident to the visit element of the provided
  /// index.
  template <typename IndexType>
  VTKM_EXEC IndicesType GetIndices(const IndexType& index) const
  {
    return Helper::GetIndices(this->Internals, index);
  }

  /// Convenience method that converts a flat, 1D index to the visited elements to a `vtkm::Vec`
  /// containing the logical indices in the grid.
  VTKM_EXEC_CONT SchedulingRangeType FlatToLogicalVisitIndex(vtkm::Id flatVisitIndex) const
  {
    return Helper::FlatToLogicalVisitIndex(this->Internals, flatVisitIndex);
  }

  /// Convenience method that converts a flat, 1D index to the incident elements to a `vtkm::Vec`
  /// containing the logical indices in the grid.
  VTKM_EXEC_CONT SchedulingRangeType FlatToLogicalIncidentIndex(vtkm::Id flatIncidentIndex) const
  {
    return Helper::FlatToLogicalIncidentIndex(this->Internals, flatIncidentIndex);
  }

  /// Convenience method that converts logical indices in a `vtkm::Vec` of a visited element
  /// to a flat, 1D index.
  VTKM_EXEC_CONT vtkm::Id LogicalToFlatVisitIndex(
    const SchedulingRangeType& logicalVisitIndex) const
  {
    return Helper::LogicalToFlatVisitIndex(this->Internals, logicalVisitIndex);
  }

  /// Convenience method that converts logical indices in a `vtkm::Vec` of an incident element
  /// to a flat, 1D index.
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

  /// Return the dimensions of the points in the cell set.
  VTKM_EXEC_CONT
  vtkm::Vec<vtkm::Id, Dimension> GetPointDimensions() const
  {
    return this->Internals.GetPointDimensions();
  }

  /// Return the dimensions of the points in the cell set.
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
