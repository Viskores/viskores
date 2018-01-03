//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_worklet_connectivity_CellSetConnectivity_h
#define vtk_m_worklet_connectivity_CellSetConnectivity_h

#include <vtkm/worklet/connectivities/CellSetDualGraph.h>
#include <vtkm/worklet/connectivities/GraphConnectivity.h>

template <typename DeviceAdapter>
class CellSetConnectivity
{
public:
  template <template <typename> class CellSetType, typename T>
  void Run(const CellSetType<T>& cellSet, vtkm::cont::ArrayHandle<vtkm::Id>& componentArray) const
  {
    vtkm::cont::ArrayHandle<vtkm::Id> numIndicesArray;
    vtkm::cont::ArrayHandle<vtkm::Id> indexOffsetArray;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivityArray;

    // create cell to cell connectivity graph (dual graph)
    CellSetDualGraph<DeviceAdapter>().Run(
      cellSet, numIndicesArray, indexOffsetArray, connectivityArray);
    // find the connected component of the dual graph
    GraphConnectivity<DeviceAdapter>().Run(
      numIndicesArray, indexOffsetArray, connectivityArray, componentArray);
  }
};
#endif // vtk_m_worklet_connectivity_CellSetConnectivity_h
