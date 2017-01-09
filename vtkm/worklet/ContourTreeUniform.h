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

#ifndef vtk_m_worklet_ContourTreeUniform_h
#define vtk_m_worklet_ContourTreeUniform_h

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/contourtree/Mesh2D_DEM_Triangulation.h>
#include <vtkm/worklet/contourtree/Mesh3D_DEM_Triangulation.h>
#include <vtkm/worklet/contourtree/MergeTree.h>
#include <vtkm/worklet/contourtree/ChainGraph.h>
#include <vtkm/worklet/contourtree/ContourTree.h>

#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif

const bool JOIN = true;
const bool SPLIT = false;
const bool JOIN_3D = true;
const bool SPLIT_3D = false;

typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

namespace vtkm {
namespace worklet {

class ContourTreeMesh2D
{
public:

  template<typename FieldType, typename StorageType, typename DeviceAdapter>
  void Run(vtkm::cont::ArrayHandle<FieldType, StorageType> fieldArray,
           vtkm::Id nRows,
           vtkm::Id nCols,
           vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id> > &saddlePeak,
           DeviceAdapter device)
  {
    vtkm::Id nSlices = 1;

    // Build the mesh and fill in the values
    contourtree::Mesh2D_DEM_Triangulation<FieldType,StorageType,DeviceAdapter> 
                                          mesh(fieldArray, device, nRows, nCols);

    // Initialize the join tree so that all arcs point to maxima
    contourtree::MergeTree<FieldType,StorageType,DeviceAdapter> 
                           joinTree(fieldArray, device, nRows, nCols, nSlices, JOIN);
    mesh.SetStarts(joinTree.extrema, JOIN);
    joinTree.BuildRegularChains();

    // Create the active topology graph from the regular graph
    contourtree::ChainGraph<FieldType,StorageType,DeviceAdapter> 
                           joinGraph(fieldArray, device, joinTree.extrema, JOIN);
    mesh.SetSaddleStarts(joinGraph, JOIN);

    // Call join graph to finish computation
    joinGraph.Compute(joinTree.saddles);

    // Initialize the split tree so that all arcs point to maxima
    contourtree::MergeTree<FieldType,StorageType,DeviceAdapter> 
                           splitTree(fieldArray, device, nRows, nCols, nSlices, SPLIT);
    mesh.SetStarts(splitTree.extrema, SPLIT);
    splitTree.BuildRegularChains();

    // Create the active topology graph from the regular graph
    contourtree::ChainGraph<FieldType,StorageType,DeviceAdapter> 
                           splitGraph(fieldArray, device, splitTree.extrema, SPLIT);
    mesh.SetSaddleStarts(splitGraph, SPLIT);

    // Call split graph to finish computation
    splitGraph.Compute(splitTree.saddles);

    // Now compute the contour tree
    contourtree::ContourTree<FieldType,StorageType,DeviceAdapter> 
                             contourTree(fieldArray, device,
                                         joinTree, joinGraph,
                                         splitTree, splitGraph);

    contourTree.CollectSaddlePeak(saddlePeak);
  }
};

class ContourTreeMesh3D
{
public:

  template<typename FieldType, typename StorageType, typename DeviceAdapter>
  void Run(vtkm::cont::ArrayHandle<FieldType, StorageType> fieldArray,
           vtkm::Id nRows,
           vtkm::Id nCols,
           vtkm::Id nSlices,
           vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id> > &saddlePeak,
           DeviceAdapter device)
  {
    // Build the mesh and fill in the values
    contourtree::Mesh3D_DEM_Triangulation<FieldType,StorageType,DeviceAdapter> 
                                          mesh(fieldArray, device, nRows, nCols, nSlices);

    // Initialize the join tree so that all arcs point to maxima
    contourtree::MergeTree<FieldType,StorageType,DeviceAdapter> 
                           joinTree(fieldArray, device, nRows, nCols, nSlices, JOIN_3D);
    mesh.SetStarts(joinTree.extrema, JOIN_3D);
    joinTree.BuildRegularChains();

    // Create the active topology graph from the regular graph
    contourtree::ChainGraph<FieldType,StorageType,DeviceAdapter> 
                           joinGraph(fieldArray, device, joinTree.extrema, JOIN_3D);
    mesh.SetSaddleStarts(joinGraph, JOIN_3D);

    // Call join graph to finish computation
    joinGraph.Compute(joinTree.saddles);

    // Initialize the split tree so that all arcs point to maxima
    contourtree::MergeTree<FieldType,StorageType,DeviceAdapter> 
                           splitTree(fieldArray, device, nRows, nCols, nSlices, SPLIT_3D);
    mesh.SetStarts(splitTree.extrema, SPLIT_3D);
    splitTree.BuildRegularChains();

    // Create the active topology graph from the regular graph
    contourtree::ChainGraph<FieldType,StorageType,DeviceAdapter> 
                           splitGraph(fieldArray, device, splitTree.extrema, SPLIT_3D);
    mesh.SetSaddleStarts(splitGraph, SPLIT_3D);

    // Call split graph to finish computation
    splitGraph.Compute(splitTree.saddles);

    // Now compute the contour tree
    contourtree::ContourTree<FieldType,StorageType,DeviceAdapter> 
                             contourTree(fieldArray, device,
                                         joinTree, joinGraph,
                                         splitTree, splitGraph);

    contourTree.CollectSaddlePeak(saddlePeak);
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_ContourTreeUniform_h
