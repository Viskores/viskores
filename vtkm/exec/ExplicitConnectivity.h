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
#ifndef vtk_m_exec_ExplicitConnectivity_h
#define vtk_m_exec_ExplicitConnectivity_h

#include <vtkm/Types.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm {
namespace exec {

template<typename Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class ExplicitConnectivity
{
public:
  ExplicitConnectivity() {}

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

#if 0
  VTKM_EXEC_EXPORT
  template <vtkm::IdComponent ItemTupleLength>
  void GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    int n = GetNumberOfIndices(index);
    int start = MapCellToConnectivityIndex.Get(index);
    for (int i=0; i<n && i<ItemTupleLength; i++)
      ids[i] = Connectivity.Get(start+i);
  }

  VTKM_EXEC_EXPORT
  template <vtkm::IdComponent ItemTupleLength>
  void AddShape(vtkm::CellType cellType, int numVertices, vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    ///\todo: how do I modify an array handle?
  }
#endif


 typedef typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<Device>::PortalConst PortalType;
 PortalType Shapes;
 PortalType NumIndices;
 PortalType Connectivity;
 PortalType MapCellToConnectivityIndex;
};

} // namespace exec
} // namespace vtkm

#endif //  vtk_m_exec_ExplicitConnectivity_h
