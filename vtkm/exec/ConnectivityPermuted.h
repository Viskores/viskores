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


#ifndef vtk_m_exec_ConnectivityPermuted_h
#define vtk_m_exec_ConnectivityPermuted_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>
#include <vtkm/exec/ConnectivityStructured.h>

namespace vtkm {
namespace exec {

template<typename PermutationPortal,
         typename OriginalConnectivity>
class ConnectivityPermuted
{
public:
  typedef vtkm::Id SchedulingRangeType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ConnectivityPermuted():
    Portal(),
    Connectivity()
  {

  }

  VTKM_EXEC_CONT
  ConnectivityPermuted(const PermutationPortal& portal,
                       const OriginalConnectivity &src):
    Portal(portal),
    Connectivity(src)
  {
  }

  VTKM_EXEC_CONT
  ConnectivityPermuted(const ConnectivityPermuted &src):
    Portal(src.Portal),
    Connectivity(src.Connectivity)
  {
  }

  VTKM_EXEC
  vtkm::IdComponent GetNumberOfIndices(vtkm::Id index) const {
    return this->Connectivity.GetNumberOfIndices( this->Portal.Get(index) );
  }


  typedef typename OriginalConnectivity::CellShapeTag CellShapeTag;

  VTKM_EXEC
  CellShapeTag GetCellShape(vtkm::Id index) const {
    vtkm::Id pIndex = this->Portal.Get(index);
    return this->Connectivity.GetCellShape( pIndex );
  }

  typedef typename OriginalConnectivity::IndicesType IndicesType;

  VTKM_EXEC
  IndicesType GetIndices(vtkm::Id index) const
  {
    return this->Connectivity.GetIndices( this->Portal.Get(index) );
  }

private:
  PermutationPortal Portal;
  OriginalConnectivity Connectivity;
};

}
} // namespace vtkm::exec

#endif //vtk_m_exec_ConnectivityPermuted_h
