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

//=======================================================================================
// 
// Second Attempt to Compute Contour Tree in Data-Parallel Mode
//
// Started August 19, 2015
//
// Copyright Hamish Carr, University of Leeds
//
// Mesh2D_DEM_Triangulation.h - class representing the topology of a 2D triangulation read
// in from a DEM - initially in ASCII text
//
//=======================================================================================
//
// COMMENTS:
//
//	Essentially, a vector of data values. BUT we will want them sorted to simplify 
//	processing - i.e. it's the robust way of handling simulation of simplicity
//
//	On the other hand, once we have them sorted, we can discard the original data since
//	only the sort order matters
// 
//	Since we've been running into memory issues, we'll start being more careful.
//	Clearly, we can eliminate the values if we sort, but in this iteration we are 
//	deferring doing a full sort, so we need to keep the values.
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_mesh3d_dem_triangulation_h
#define vtkm_worklet_contourtree_mesh3d_dem_triangulation_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/worklet/contourtree/Types.h>
#include <vtkm/worklet/contourtree/ChainGraph.h>
#include <vtkm/worklet/contourtree/PrintVectors.h>
#include <vtkm/worklet/contourtree/Mesh3D_DEM_VertexStarter.h>
#include <vtkm/worklet/contourtree/Mesh3D_DEM_VertexOutdegreeStarter.h>
#include <vtkm/worklet/contourtree/Mesh3D_DEM_SaddleStarter.h>
#include <vtkm/worklet/contourtree/LinkComponentCaseTable3D.h>

#define DEBUG_PRINT 1
//#define DEBUG_TIMING 1

namespace vtkm {
namespace worklet {
namespace contourtree {

template <typename T, typename StorageType, typename DeviceAdapter>
class Mesh3D_DEM_Triangulation
{
public:
  typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;

  // size of the mesh
  vtkm::Id nRows, nCols, nSlices, nVertices, nLogSteps;

  // original data array
  const vtkm::cont::ArrayHandle<T,StorageType> &values;

  // device
  DeviceAdapter device;

  // array with neighbourhood masks
  vtkm::cont::ArrayHandle<vtkm::Id> neighbourhoodMask;

  // case table information for finding neighbours
  vtkm::cont::ArrayHandle<vtkm::UInt16> linkComponentCaseTable3D;
	
  // constructor
  Mesh3D_DEM_Triangulation(const vtkm::cont::ArrayHandle<T,StorageType> &Values,
                           DeviceAdapter Device,
                           vtkm::Id NRows,
                           vtkm::Id NCols,
                           vtkm::Id NSlices);

  // sets all vertices to point along an outgoing edge (except extrema)
  void SetStarts(vtkm::cont::ArrayHandle<vtkm::Id> &chains,
                 bool descending);

  // sets outgoing paths for saddles
  void SetSaddleStarts(ChainGraph<T,StorageType,DeviceAdapter> &mergeGraph, bool descending);
};

// creates input mesh
template<typename T, typename StorageType, typename DeviceAdapter>
Mesh3D_DEM_Triangulation<T,StorageType,DeviceAdapter>::Mesh3D_DEM_Triangulation(
                           const vtkm::cont::ArrayHandle<T,StorageType> &Values,
                           DeviceAdapter Device,
                           vtkm::Id NRows,
                           vtkm::Id NCols,
                           vtkm::Id NSlices) :
                               values(Values),
                               device(Device),
                               nRows(NRows),
                               nCols(NCols),
                               nSlices(NSlices),
                               linkComponentCaseTable3D()
{
  nVertices = nRows * nCols * nSlices;
  
  // compute the number of log-jumping steps (i.e. lg_2 (nVertices))
  nLogSteps = 1;
  for (signed long shifter = nVertices; shifter > 0; shifter >>= 1)
    nLogSteps++;

  linkComponentCaseTable3D = 
       vtkm::cont::make_ArrayHandle(vtkm::worklet::contourtree::linkComponentCaseTable3D, 16384);
}

// sets outgoing paths for saddles
template<typename T, typename StorageType, typename DeviceAdapter>
void Mesh3D_DEM_Triangulation<T,StorageType,DeviceAdapter>::SetStarts(
                                         vtkm::cont::ArrayHandle<vtkm::Id> &chains, 
                                         bool ascending)
{
  // create the neighbourhood mask
  neighbourhoodMask.Allocate(nVertices);

  // For each vertex set the next vertex in the chain
  vtkm::cont::ArrayHandleIndex vertexIndexArray(nVertices);
  Mesh3D_DEM_VertexStarter<T> vertexStarter(nRows, nCols, nSlices, ascending);
  vtkm::worklet::DispatcherMapField<Mesh3D_DEM_VertexStarter<T> > 
                 vertexStarterDispatcher(vertexStarter); 

  vertexStarterDispatcher.Invoke(vertexIndexArray,         // input
                                 values,                   // input (whole array)
                                 chains,                   // output
                                 neighbourhoodMask);       // output
} // SetStarts()

// sets outgoing paths for saddles
template<typename T, typename StorageType, typename DeviceAdapter>
void Mesh3D_DEM_Triangulation<T,StorageType,DeviceAdapter>::SetSaddleStarts(ChainGraph<T,StorageType,DeviceAdapter> &mergeGraph,
                                               bool ascending)
{
  // we need a temporary inverse index to change vertex IDs 
  vtkm::cont::ArrayHandle<vtkm::Id> inverseIndex;
  vtkm::cont::ArrayHandle<vtkm::Id> isCritical;
  vtkm::cont::ArrayHandle<vtkm::Id> outdegree;
  inverseIndex.Allocate(nVertices);
  isCritical.Allocate(nVertices);
  outdegree.Allocate(nVertices);
cout << "SetSaddleStarts nVertices " << nVertices << endl;

  vtkm::cont::ArrayHandleIndex vertexIndexArray(nVertices);
  Mesh3D_DEM_VertexOutdegreeStarter<DeviceAdapter> 
             vertexOutdegreeStarter(nRows, 
                                    nCols, 
                                    nSlices, 
                                    ascending,
                                    linkComponentCaseTable3D.PrepareForInput(device));
  vtkm::worklet::DispatcherMapField<Mesh3D_DEM_VertexOutdegreeStarter<DeviceAdapter> > 
                 vertexOutdegreeStarterDispatcher(vertexOutdegreeStarter); 

  vertexOutdegreeStarterDispatcher.Invoke(vertexIndexArray,         // input
                                          neighbourhoodMask,        // input
                                          mergeGraph.arcArray,      // input (whole array)
                                          outdegree,                // output
                                          isCritical);              // output

  DeviceAlgorithm::ScanExclusive(isCritical, inverseIndex);
		
  // now we can compute how many critical points we carry forward
  vtkm::Id nCriticalPoints = inverseIndex.GetPortalConstControl().Get(nVertices-1) +
                             isCritical.GetPortalConstControl().Get(nVertices-1);
cout << "SetSaddleStarts inverseIndex last vert " << inverseIndex.GetPortalConstControl().Get(nVertices-1) << endl;
cout << "SetSaddleStarts isCritical last vert " << isCritical.GetPortalConstControl().Get(nVertices-1) << endl;
cout << "SetSaddleStarts nCriticalPoints " << nCriticalPoints << endl;

  // allocate space for the join graph vertex arrays
  mergeGraph.AllocateVertexArrays(nCriticalPoints);
		
  // compact the set of vertex indices to critical ones only
  DeviceAlgorithm::StreamCompact(vertexIndexArray, isCritical, mergeGraph.valueIndex);

  // we initialise the prunesTo array to "NONE"
  vtkm::cont::ArrayHandleConstant<vtkm::Id> notAssigned(NO_VERTEX_ASSIGNED, nCriticalPoints);
  DeviceAlgorithm::Copy(notAssigned, mergeGraph.prunesTo);

  // copy the outdegree from our temporary array
  // : mergeGraph.outdegree[vID] <= outdegree[mergeGraph.valueIndex[vID]]
  DeviceAlgorithm::StreamCompact(outdegree, isCritical, mergeGraph.outdegree);

  // copy the chain maximum from arcArray
  // : mergeGraph.chainExtremum[vID] = inverseIndex[mergeGraph.arcArray[mergeGraph.valueIndex[vID]]]	
  typedef vtkm::cont::ArrayHandle<vtkm::Id> IdArrayType;
  typedef vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType> PermuteIndexType;

  vtkm::cont::ArrayHandle<vtkm::Id> tArray;
  tArray.Allocate(nCriticalPoints);
  DeviceAlgorithm::StreamCompact(mergeGraph.arcArray, isCritical, tArray);
  DeviceAlgorithm::Copy(PermuteIndexType(tArray, inverseIndex), mergeGraph.chainExtremum);

  // and set up the active vertices - initially to identity
  vtkm::cont::ArrayHandleIndex criticalVertsIndexArray(nCriticalPoints);
  DeviceAlgorithm::Copy(criticalVertsIndexArray, mergeGraph.activeVertices);
    
  // now we need to compute the firstEdge array from the outdegrees
  DeviceAlgorithm::ScanExclusive(mergeGraph.outdegree, mergeGraph.firstEdge);

  vtkm::Id nCriticalEdges = mergeGraph.firstEdge.GetPortalConstControl().Get(nCriticalPoints-1) +
                            mergeGraph.outdegree.GetPortalConstControl().Get(nCriticalPoints-1);

  // now we allocate the edge arrays
  mergeGraph.AllocateEdgeArrays(nCriticalEdges);

  // and we have to set them, so we go back to the vertices
  Mesh3D_DEM_SaddleStarter<DeviceAdapter> 
             saddleStarter(nRows,                     // input
                           nCols,                     // input
                           nSlices,                   // input
                           ascending,                 // input
                           linkComponentCaseTable3D.PrepareForInput(device));
  vtkm::worklet::DispatcherMapField<Mesh3D_DEM_SaddleStarter<DeviceAdapter> > 
                 saddleStarterDispatcher(saddleStarter); 

  vtkm::cont::ArrayHandleZip<vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::cont::ArrayHandle<vtkm::Id> > outDegFirstEdge =
              vtkm::cont::make_ArrayHandleZip(mergeGraph.outdegree, mergeGraph.firstEdge);

  saddleStarterDispatcher.Invoke(criticalVertsIndexArray,           // input
                                 outDegFirstEdge,                   // input (pair)
                                 mergeGraph.valueIndex,             // input
                                 neighbourhoodMask,                 // input (whole array)
                                 mergeGraph.arcArray,               // input (whole array)
                                 inverseIndex,                      // input (whole array)
                                 mergeGraph.edgeNear,               // output (whole array)
                                 mergeGraph.edgeFar,                // output (whole array)
                                 mergeGraph.activeEdges);           // output (whole array)

  // finally, allocate and initialise the edgeSorter array
  DeviceAlgorithm::Copy(mergeGraph.activeEdges, mergeGraph.edgeSorter);
} // SetSaddleStarts()

}
}
}

#endif
