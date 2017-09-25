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

#ifndef vtk_m_exec_ConnectivityStructured_h
#define vtk_m_exec_ConnectivityStructured_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>
#include <vtkm/internal/ConnectivityStructuredInternals.h>

namespace vtkm
{
namespace exec
{

template <typename FromTopology, typename ToTopology, vtkm::IdComponent Dimension>
class ConnectivityStructured
{
  VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
  VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);

  using InternalsType = vtkm::internal::ConnectivityStructuredInternals<Dimension>;

  using Helper =
    vtkm::internal::ConnectivityStructuredIndexHelper<FromTopology, ToTopology, Dimension>;

public:
  using SchedulingRangeType = typename InternalsType::SchedulingRangeType;

  VTKM_EXEC_CONT
  ConnectivityStructured()
    : Internals()
  {
  }

  VTKM_EXEC_CONT
  ConnectivityStructured(const InternalsType& src)
    : Internals(src)
  {
  }

  VTKM_EXEC_CONT
  ConnectivityStructured(const ConnectivityStructured& src)
    : Internals(src.Internals)
  {
  }

  VTKM_EXEC_CONT
  ConnectivityStructured(const ConnectivityStructured<ToTopology, FromTopology, Dimension>& src)
    : Internals(src.Internals)
  {
  }

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

  VTKM_EXEC_CONT
  SchedulingRangeType FlatToLogicalFromIndex(vtkm::Id flatFromIndex) const
  {
    return Helper::FlatToLogicalFromIndex(this->Internals, flatFromIndex);
  }

  VTKM_EXEC_CONT
  vtkm::Id LogicalToFlatFromIndex(const SchedulingRangeType& logicalFromIndex) const
  {
    return Helper::LogicalToFlatFromIndex(this->Internals, logicalFromIndex);
  }

  VTKM_EXEC_CONT
  SchedulingRangeType FlatToLogicalToIndex(vtkm::Id flatToIndex) const
  {
    return Helper::FlatToLogicalToIndex(this->Internals, flatToIndex);
  }

  VTKM_EXEC_CONT
  vtkm::Id LogicalToFlatToIndex(const SchedulingRangeType& logicalToIndex) const
  {
    return Helper::LogicalToFlatToIndex(this->Internals, logicalToIndex);
  }

  VTKM_EXEC_CONT
  vtkm::Vec<vtkm::Id, Dimension> GetPointDimensions() const
  {
    return this->Internals.GetPointDimensions();
  }

  friend class ConnectivityStructured<ToTopology, FromTopology, Dimension>;

private:
  InternalsType Internals;
};
}
} // namespace vtkm::exec

#endif //vtk_m_exec_ConnectivityStructured_h
