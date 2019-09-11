//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

//  This code is based on the algorithm presented in the paper:
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.”
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.

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

#ifndef vtkm_worklet_contourtree_mesh2d_dem_triangulation_h
#define vtkm_worklet_contourtree_mesh2d_dem_triangulation_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/worklet/contourtree/ChainGraph.h>
#include <vtkm/worklet/contourtree/Mesh2D_DEM_SaddleStarter.h>
#include <vtkm/worklet/contourtree/Mesh2D_DEM_VertexOutdegreeStarter.h>
#include <vtkm/worklet/contourtree/Mesh2D_DEM_VertexStarter.h>
#include <vtkm/worklet/contourtree/PrintVectors.h>
#include <vtkm/worklet/contourtree/Types.h>

//#define DEBUG_PRINT 1
//#define DEBUG_TIMING 1

namespace vtkm
{
namespace worklet
{
namespace contourtree
{

template <typename T, typename StorageType>
class Mesh2D_DEM_Triangulation
{
public:
  // original data array
  const vtkm::cont::ArrayHandle<T, StorageType>& values;

  // size of the mesh
  vtkm::Id nRows, nCols, nVertices, nLogSteps;

  // Array with neighbourhood masks
  vtkm::cont::ArrayHandle<vtkm::Id> neighbourhoodMask;

  // constructor
  Mesh2D_DEM_Triangulation(const vtkm::cont::ArrayHandle<T, StorageType>& Values,
                           vtkm::Id NRows,
                           vtkm::Id NCols);

  // sets all vertices to point along an outgoing edge (except extrema)
  void SetStarts(vtkm::cont::ArrayHandle<vtkm::Id>& chains, bool descending);

  // sets outgoing paths for saddles
  void SetSaddleStarts(ChainGraph<T, StorageType>& mergeGraph, bool descending);
};

// sets outgoing paths for saddles
template <typename T, typename StorageType>
void Mesh2D_DEM_Triangulation<T, StorageType>::SetStarts(vtkm::cont::ArrayHandle<vtkm::Id>& chains,
                                                         bool ascending)
{
  // create the neighbourhood mask
  neighbourhoodMask.Allocate(nVertices);

  // For each vertex set the next vertex in the chain
  vtkm::cont::ArrayHandleIndex vertexIndexArray(nVertices);
  Mesh2D_DEM_VertexStarter<T> vertexStarter(nRows, nCols, ascending);
  vtkm::worklet::DispatcherMapField<Mesh2D_DEM_VertexStarter<T>> vertexStarterDispatcher(
    vertexStarter);

  vertexStarterDispatcher.Invoke(vertexIndexArray,   // input
                                 values,             // input (whole array)
                                 chains,             // output
                                 neighbourhoodMask); // output
} // SetStarts()

// creates input mesh
template <typename T, typename StorageType>
Mesh2D_DEM_Triangulation<T, StorageType>::Mesh2D_DEM_Triangulation(
  const vtkm::cont::ArrayHandle<T, StorageType>& Values,
  vtkm::Id NRows,
  vtkm::Id NCols)
  : values(Values)
  , nRows(NRows)
  , nCols(NCols)
{
  nVertices = nRows * nCols;

  // compute the number of log-jumping steps (i.e. lg_2 (nVertices))
  nLogSteps = 1;
  for (vtkm::Id shifter = nVertices; shifter > 0; shifter >>= 1)
    nLogSteps++;
}

// sets outgoing paths for saddles
template <typename T, typename StorageType>
void Mesh2D_DEM_Triangulation<T, StorageType>::SetSaddleStarts(
  ChainGraph<T, StorageType>& mergeGraph,
  bool ascending)
{
  // we need a temporary inverse index to change vertex IDs
  vtkm::cont::ArrayHandle<vtkm::Id> inverseIndex;
  vtkm::cont::ArrayHandle<vtkm::Id> isCritical;
  vtkm::cont::ArrayHandle<vtkm::Id> outdegree;
  inverseIndex.Allocate(nVertices);
  isCritical.Allocate(nVertices);
  outdegree.Allocate(nVertices);

  vtkm::cont::ArrayHandleIndex vertexIndexArray(nVertices);
  Mesh2D_DEM_VertexOutdegreeStarter vertexOutdegreeStarter(nRows, nCols, ascending);
  vtkm::worklet::DispatcherMapField<Mesh2D_DEM_VertexOutdegreeStarter>
    vertexOutdegreeStarterDispatcher(vertexOutdegreeStarter);

  vertexOutdegreeStarterDispatcher.Invoke(vertexIndexArray,    // input
                                          neighbourhoodMask,   // input
                                          mergeGraph.arcArray, // input (whole array)
                                          outdegree,           // output
                                          isCritical);         // output

  vtkm::cont::Algorithm::ScanExclusive(isCritical, inverseIndex);

  // now we can compute how many critical points we carry forward
  vtkm::Id nCriticalPoints = vtkm::cont::ArrayGetValue(nVertices - 1, inverseIndex) +
    vtkm::cont::ArrayGetValue(nVertices - 1, isCritical);

  // allocate space for the join graph vertex arrays
  mergeGraph.AllocateVertexArrays(nCriticalPoints);

  // compact the set of vertex indices to critical ones only
  vtkm::cont::Algorithm::CopyIf(vertexIndexArray, isCritical, mergeGraph.valueIndex);

  // we initialise the prunesTo array to "NONE"
  vtkm::cont::ArrayHandleConstant<vtkm::Id> notAssigned(NO_VERTEX_ASSIGNED, nCriticalPoints);
  vtkm::cont::Algorithm::Copy(notAssigned, mergeGraph.prunesTo);

  // copy the outdegree from our temporary array
  // : mergeGraph.outdegree[vID] <= outdegree[mergeGraph.valueIndex[vID]]
  vtkm::cont::Algorithm::CopyIf(outdegree, isCritical, mergeGraph.outdegree);

  // copy the chain maximum from arcArray
  // : mergeGraph.chainExtremum[vID] = inverseIndex[mergeGraph.arcArray[mergeGraph.valueIndex[vID]]]
  using IdArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using PermuteIndexType = vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  vtkm::cont::ArrayHandle<vtkm::Id> tArray;
  tArray.Allocate(nCriticalPoints);
  vtkm::cont::Algorithm::CopyIf(mergeGraph.arcArray, isCritical, tArray);
  vtkm::cont::Algorithm::Copy(PermuteIndexType(tArray, inverseIndex), mergeGraph.chainExtremum);

  // and set up the active vertices - initially to identity
  vtkm::cont::ArrayHandleIndex criticalVertsIndexArray(nCriticalPoints);
  vtkm::cont::Algorithm::Copy(criticalVertsIndexArray, mergeGraph.activeVertices);

  // now we need to compute the firstEdge array from the outdegrees
  vtkm::cont::Algorithm::ScanExclusive(mergeGraph.outdegree, mergeGraph.firstEdge);

  vtkm::Id nCriticalEdges = vtkm::cont::ArrayGetValue(nCriticalPoints - 1, mergeGraph.firstEdge) +
    vtkm::cont::ArrayGetValue(nCriticalPoints - 1, mergeGraph.outdegree);

  // now we allocate the edge arrays
  mergeGraph.AllocateEdgeArrays(nCriticalEdges);

  // and we have to set them, so we go back to the vertices
  Mesh2D_DEM_SaddleStarter saddleStarter(nRows,      // input
                                         nCols,      // input
                                         ascending); // input
  vtkm::worklet::DispatcherMapField<Mesh2D_DEM_SaddleStarter> saddleStarterDispatcher(
    saddleStarter);

  vtkm::cont::ArrayHandleZip<vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::cont::ArrayHandle<vtkm::Id>>
    outDegFirstEdge = vtkm::cont::make_ArrayHandleZip(mergeGraph.outdegree, mergeGraph.firstEdge);

  saddleStarterDispatcher.Invoke(criticalVertsIndexArray, // input
                                 outDegFirstEdge,         // input (pair)
                                 mergeGraph.valueIndex,   // input
                                 neighbourhoodMask,       // input (whole array)
                                 mergeGraph.arcArray,     // input (whole array)
                                 inverseIndex,            // input (whole array)
                                 mergeGraph.edgeNear,     // output (whole array)
                                 mergeGraph.edgeFar,      // output (whole array)
                                 mergeGraph.activeEdges); // output (whole array)

  // finally, allocate and initialise the edgeSorter array
  vtkm::cont::ArrayCopy(mergeGraph.activeEdges, mergeGraph.edgeSorter);
} // SetSaddleStarts()
}
}
}

#endif
