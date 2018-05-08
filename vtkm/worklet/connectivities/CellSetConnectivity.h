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

namespace vtkm
{
namespace worklet
{
namespace connectivity
{

class CellSetConnectivity
{
public:
  template <typename CellSetType, typename DeviceAdapter>
  void Run(const CellSetType& cellSet,
           vtkm::cont::ArrayHandle<vtkm::Id>& componentArray,
           DeviceAdapter) const
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
}
}
} // vtkm::worklet::connectivity

#endif // vtk_m_worklet_connectivity_CellSetConnectivity_h
