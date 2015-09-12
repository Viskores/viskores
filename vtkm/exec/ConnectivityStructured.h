//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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

namespace vtkm {
namespace exec {

template<typename FromTopology,
         typename ToTopology,
         vtkm::IdComponent Dimension>
class ConnectivityStructured
{
  VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
  VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);

  typedef vtkm::internal::ConnectivityStructuredInternals<Dimension>
      InternalsType;

  typedef vtkm::internal::ConnectivityStructuredIndexHelper<
      FromTopology,ToTopology,Dimension> Helper;
public:
  typedef typename InternalsType::SchedulingRangeType SchedulingRangeType;

  VTKM_EXEC_CONT_EXPORT
  ConnectivityStructured():
    Internals()
  {

  }

  VTKM_EXEC_CONT_EXPORT
  ConnectivityStructured(const InternalsType &src):
    Internals(src)
  {
  }

  VTKM_EXEC_CONT_EXPORT
  ConnectivityStructured(const ConnectivityStructured &src):
    Internals(src.Internals)
  {
  }

  VTKM_EXEC_EXPORT
  vtkm::IdComponent GetNumberOfIndices(vtkm::Id index) const {
    return Helper::GetNumberOfIndices(this->Internals, index);
  }

  // This needs some thought. What does cell shape mean when the to topology
  // is not a cell?
  typedef typename InternalsType::CellShapeTag CellShapeTag;
  VTKM_EXEC_EXPORT
  CellShapeTag GetCellShape(vtkm::Id=0) const {
    return CellShapeTag();
  }

  typedef typename Helper::IndicesType IndicesType;

  VTKM_EXEC_EXPORT
  IndicesType GetIndices(vtkm::Id index) const
  {
    return Helper::GetIndices(this->Internals,index);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Vec<vtkm::Id,Dimension>
  FlatToLogicalPointIndex(vtkm::Id flatPointIndex) const
  {
    return this->Internals.FlatToLogicalPointIndex(flatPointIndex);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id LogicalToFlatPointIndex(
      const vtkm::Vec<vtkm::Id,Dimension> &logicalPointIndex) const
  {
    return this->Internals.LogicalToFlatPointIndex(logicalPointIndex);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Vec<vtkm::Id,Dimension>
  FlatToLogicalCellIndex(vtkm::Id flatCellIndex) const
  {
    return this->Internals.FlatToLogicalCellIndex(flatCellIndex);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id LogicalToFlatCellIndex(
      const vtkm::Vec<vtkm::Id,Dimension> &logicalCellIndex) const
  {
    return this->Internals.LogicalToFlatCellIndex(logicalCellIndex);
  }

private:
  InternalsType Internals;
};

}
} // namespace vtkm::exec

#endif //vtk_m_exec_ConnectivityStructured_h
