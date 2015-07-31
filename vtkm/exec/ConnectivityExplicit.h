//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_ConnectivityExplicit_h
#define vtk_m_exec_ConnectivityExplicit_h

#include <vtkm/Types.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm {
namespace exec {

template<typename ShapePortalType,
         typename NumIndicesPortalType,
         typename ConnectivityPortalType,
         typename MapConnectivityPortalType
         >
class ConnectivityExplicit
{
public:
  ConnectivityExplicit() {}

  ConnectivityExplicit(const ShapePortalType& shapePortal,
                       const NumIndicesPortalType& indicePortal,
                       const ConnectivityPortalType& connPortal,
                       const MapConnectivityPortalType& mapConnPortal
                       )
  : Shapes(shapePortal),
    NumIndices(indicePortal),
    Connectivity(connPortal),
    MapCellToConnectivityIndex(mapConnPortal)
  {

  }

  VTKM_EXEC_EXPORT
  vtkm::Id GetNumberOfElements()
  {
      return Shapes.GetNumberOfValues();
  }

  VTKM_EXEC_EXPORT
  vtkm::Id GetNumberOfIndices(vtkm::Id index)
  {
      return NumIndices.Get(index);
  }

  VTKM_EXEC_EXPORT
  vtkm::Id GetElementShapeType(vtkm::Id index)
  {
      return Shapes.Get(index);
  }

  template <vtkm::IdComponent ItemTupleLength>
  VTKM_EXEC_EXPORT
  void GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    vtkm::Id n = GetNumberOfIndices(index);
    vtkm::Id start = MapCellToConnectivityIndex.Get(index);
    for (vtkm::IdComponent i=0; i<n && i<ItemTupleLength; i++)
      ids[i] = Connectivity.Get(start+i);
  }

private:
 ShapePortalType Shapes;
 NumIndicesPortalType NumIndices;
 ConnectivityPortalType Connectivity;
 MapConnectivityPortalType MapCellToConnectivityIndex;
};

} // namespace exec
} // namespace vtkm

#endif //  vtk_m_exec_ConnectivityExplicit_h
