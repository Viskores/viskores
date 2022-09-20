//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_connectivity_CellSetConnectivity_h
#define vtk_m_worklet_connectivity_CellSetConnectivity_h

#include <vtkm/filter/connected_components/worklet/CellSetDualGraph.h>
#include <vtkm/filter/connected_components/worklet/GraphConnectivity.h>

namespace vtkm
{
namespace worklet
{
namespace connectivity
{

class CellSetConnectivity
{
public:
  static void Run(const vtkm::cont::UnknownCellSet& cellSet,
                  vtkm::cont::ArrayHandle<vtkm::Id>& componentArray)
  {
    vtkm::cont::ArrayHandle<vtkm::Id> numIndicesArray;
    vtkm::cont::ArrayHandle<vtkm::Id> indexOffsetsArray;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivityArray;

    // create cell to cell connectivity graph (dual graph)
    CellSetDualGraph::Run(cellSet, numIndicesArray, indexOffsetsArray, connectivityArray);
    // find the connected component of the dual graph
    GraphConnectivity::Run(numIndicesArray, indexOffsetsArray, connectivityArray, componentArray);
  }
};
}
}
} // vtkm::worklet::connectivity

#endif // vtk_m_worklet_connectivity_CellSetConnectivity_h
