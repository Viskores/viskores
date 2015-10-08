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


#ifndef vtk_m_exec_ConnectivityStructuredPermuted_h
#define vtk_m_exec_ConnectivityStructuredPermuted_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>
#include <vtkm/exec/ConnectivityStructured.h>

namespace vtkm {
namespace exec {

template<typename PermutationPortal,
         typename FromTopology,
         typename ToTopology,
         vtkm::IdComponent Dimension>
class ConnectivityStructuredPermuted
{
  VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
  VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);

  typedef vtkm::exec::ConnectivityStructured<FromTopology,
                                             ToTopology,
                                             Dimension> StructuredType;
public:
  typedef vtkm::Id SchedulingRangeType;

  VTKM_EXEC_CONT_EXPORT
  ConnectivityStructuredPermuted():
    Portal(),
    FullStructuredGrid()
  {

  }

  VTKM_EXEC_CONT_EXPORT
  ConnectivityStructuredPermuted(const PermutationPortal& portal,
                                 const StructuredType &src):
    Portal(portal),
    FullStructuredGrid(src)
  {
  }

  VTKM_EXEC_CONT_EXPORT
  ConnectivityStructuredPermuted(const ConnectivityStructuredPermuted &src):
    Portal(src.Portal),
    FullStructuredGrid(src.FullStructuredGrid)
  {
  }

  VTKM_EXEC_EXPORT
  vtkm::IdComponent GetNumberOfIndices(vtkm::Id index) const {
    return this->FullStructuredGrid.GetNumberOfIndices( this->Portal.Get(index) );
  }

  // This needs some thought. What does cell shape mean when the to topology
  // is not a cell?
  typedef typename StructuredType::CellShapeTag CellShapeTag;
  VTKM_EXEC_EXPORT
  CellShapeTag GetCellShape(vtkm::Id=0) const {
    return CellShapeTag();
  }

  typedef typename StructuredType::IndicesType IndicesType;

  VTKM_EXEC_EXPORT
  IndicesType GetIndices(vtkm::Id index) const
  {
    return this->FullStructuredGrid.GetIndices( this->Portal.Get(index) );
  }

private:
  PermutationPortal Portal;
  StructuredType FullStructuredGrid;
};

}
} // namespace vtkm::exec

#endif //vtk_m_exec_ConnectivityStructuredPermuted_h
