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
// Mesh2D_DEM_SaddleStarter.h - fills in saddle ascents per vertex
//
//=======================================================================================
//
// COMMENTS:
//
// This functor replaces a parallel loop examining neighbours - again, for arbitrary
// meshes, it needs to be a reduction, but for regular meshes, it's faster this way.
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_mesh3d_dem_saddle_starter_h
#define vtkm_worklet_contourtree_mesh3d_dem_saddle_starter_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Mesh3D_DEM_Triangulation_Macros.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for setting initial chain maximum value
template<typename DeviceAdapter>
class Mesh3D_DEM_SaddleStarter : public vtkm::worklet::WorkletMapField
{
public:
  struct PairType : vtkm::ListTagBase<vtkm::Pair<vtkm::Id, vtkm::Id> > {};

  typedef void ControlSignature(FieldIn<IdType> vertex,             // (input) index into active vertices
                                FieldIn<PairType> outDegFirstEdge,  // (input) out degree/first edge of vertex
                                FieldIn<IdType> valueIndex,         // (input) index into regular graph
                                WholeArrayIn<IdType> linkMask,      // (input) neighbors of vertex
                                WholeArrayIn<IdType> arcArray,      // (input) chain extrema per vertex
                                WholeArrayIn<IdType> inverseIndex,  // (input) permutation of index
                                WholeArrayOut<IdType> edgeNear,     // (output) low end of edges
                                WholeArrayOut<IdType> edgeFar,      // (output) high end of edges
                                WholeArrayOut<IdType> activeEdges); // (output) active edge list
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8, _9);
  typedef _1   InputDomain;

  typedef typename vtkm::cont::ArrayHandle<vtkm::UInt16>::template ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;

  IdPortalType caseTable;                          // (input) case table for neighbours
  vtkm::Id nRows;                                  // (input) number of rows in 3D
  vtkm::Id nCols;                                  // (input) number of cols in 3D
  vtkm::Id nSlices;                                // (input) number of cols in 3D
  bool ascending;                                  // (input) ascending or descending (join or split)

  const vtkm::IdComponent neighbourOffsets3D[N_INCIDENT_EDGES_3D][3] = {
    { -1, -1, -1 }, { 0, -1, 0 }, { -1, -1, 0 }, { -1, 0, 0 }, { -1, 0, -1 }, { 0, 0, -1 }, { 0, -1, -1 },
    { 0, 0, 1 }, { 0, 1, 0 }, { 0, 1, 1 }, { 1, 0, 0 }, { 1, 0, 1 }, { 1, 1, 0 }, { 1, 1, 1 }
  };

  // Constructor
  VTKM_EXEC_CONT
  Mesh3D_DEM_SaddleStarter(vtkm::Id NRows,
                           vtkm::Id NCols,
                           vtkm::Id NSlices,
                           bool Ascending,
                           IdPortalType CaseTable) :
                                             nRows(NRows),
                                             nCols(NCols),
                                             nSlices(NSlices),
                                             ascending(Ascending),
                                             caseTable(CaseTable) {}

  // operator() routine that executes the loop
  template<typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& vertex,
                  const vtkm::Pair<vtkm::Id,vtkm::Id>& outDegFirstEdge,
                  const vtkm::Id& valueIndex,
                  const InFieldPortalType& linkMask,
                  const InFieldPortalType& arcArray,
                  const InFieldPortalType& inverseIndex,
                  const OutFieldPortalType& edgeNear,
                  const OutFieldPortalType& edgeFar,
                  const OutFieldPortalType& activeEdges) const
  {
    vtkm::Id outdegree = outDegFirstEdge.first;
    vtkm::Id firstEdge = outDegFirstEdge.second;
    // skip local extrema
    if (outdegree == 0)
      return;

    // get the saddle mask for the vertex
    vtkm::Id nbrMask = linkMask.Get(valueIndex);
		
    // get the row and column
    vtkm::Id row = VERTEX_ROW_3D(valueIndex, nRows, nCols);
    vtkm::Id col = VERTEX_COL_3D(valueIndex, nRows, nCols);
    vtkm::Id slice = VERTEX_SLICE_3D(valueIndex, nRows, nCols);
		
    // we know which edges are outbound, so we count to get the outdegree
    vtkm::Id outDegree = 0;
    vtkm::Id farEnds[MAX_OUTDEGREE_3D];

    for (vtkm::Id edgeNo = 0; edgeNo < N_INCIDENT_EDGES_3D; edgeNo++)
    {
      if (caseTable.Get(nbrMask) & (1 << edgeNo))
      {
        vtkm::Id nbrSlice = slice + neighbourOffsets3D[edgeNo][0];
        vtkm::Id nbrRow   = row   + neighbourOffsets3D[edgeNo][1];
        vtkm::Id nbrCol   = col   + neighbourOffsets3D[edgeNo][2];
        vtkm::Id nbr = VERTEX_ID_3D(nbrSlice, nbrRow, nbrCol, nRows, nCols);

        farEnds[outDegree++] = inverseIndex.Get(arcArray.Get(nbr));
      }
    }

    // now we check them against each other
    if ((outDegree == 2) && (farEnds[0] == farEnds[1]))
    { // outDegree 2 & both match
      // treat as a regular point
      outDegree = 1;
    } // outDegree 2 & both match
    else if (outDegree == 3)
    { // outDegree 3
      if (farEnds[0] == farEnds[1])
      { // first two match
        if (farEnds[0] == farEnds[2])
        { // triple match
          // all match - treat as regular point
          outDegree = 1;
        } // triple match
        else
        { // first two match, but not third
          // copy third down one place
          farEnds[1] = farEnds[2];
          // and reset the count
          outDegree = 2;
        } //
      } // first two match
      else if ((farEnds[0] == farEnds[2]) || (farEnds[1] == farEnds[2]))
      { // second one matches either of the first two
        // decrease the count, keeping 0 & 1
        outDegree = 2;
      } // second one matches either of the first two
    } // outDegree 3

    // now the farEnds array holds the far ends we can reach
    for (vtkm::Id edge = 0; edge < outDegree; edge++)
    {
      // compute the edge index in the edge arrays
      vtkm::Id edgeID = firstEdge + edge;
			
      // now set the near and far ends and save the edge itself
      edgeNear.Set(edgeID, vertex);
      edgeFar.Set(edgeID, farEnds[edge]);
      activeEdges.Set(edgeID,  edgeID);
    } // per start
  } // operator()
		
}; // Mesh3D_DEM_SaddleStarter

}
}
}

#endif
