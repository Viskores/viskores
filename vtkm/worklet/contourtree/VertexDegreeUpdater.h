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
// VertexDegreeUpdater.h - functor that computes modified updegree for vertex in graph
//
//=======================================================================================
//
// COMMENTS:
//
// This functor identifies for each vertex which edges to keep. For arbitrary meshes, 
// this should use reductions. For regular meshes, this way is faster due to low bounded
// updegree.
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_vertex_degree_updater_h
#define vtkm_worklet_contourtree_vertex_degree_updater_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for setting initial chain maximum value
class VertexDegreeUpdater : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertexID,             // (input) active vertices
                                WholeArrayIn<IdType> activeEdges,     // (input) active edges
                                WholeArrayIn<IdType> edgeFar,         // (input) high ends of edges
                                WholeArrayIn<IdType> firstEdge,       // (input) first edge for each active vertex
                                WholeArrayIn<IdType> prunesTo,        // (input) where vertex is pruned to
                                WholeArrayIn<IdType> outdegree,       // (input) updegree of vertex
                                WholeArrayInOut<IdType> chainExtemum, // (i/o) chain extemum for vertices
                                FieldOut<IdType> newOutdegree);       // (output) new updegree of vertex
  typedef _8 ExecutionSignature(_1, _2, _3, _4, _5, _6, _7);
  typedef _1 InputDomain;

  // chainMaximum is safe for I/O here because:
  // 		we have previously eliminated maxima from the active vertex list
  //		our lookup uses the chainMaximum of the edgeHigh, which is guaranteed to 
  //		be a maximum
  //		therefore, the chainMaximum entries edited are *NEVER* also accessed & v.v.

  // Constructor
  VTKM_EXEC_CONT
  VertexDegreeUpdater() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  vtkm::Id operator()(const vtkm::Id &vertexID,
                      const InFieldPortalType &activeEdges,
                      const InFieldPortalType &edgeFar,
                      const InFieldPortalType &firstEdge,
                      const InFieldPortalType &prunesTo,
                      const InFieldPortalType &outdegree,
                      const OutFieldPortalType &chainExtremum) const
  {
    vtkm::Id newOutdegree = 0;

    // retrieve actual vertex ID & first edge
    vtkm::Id edgeFirst = firstEdge.Get(vertexID);

    // also reset the chain maximum to the vertex' ID
    chainExtremum.Set(vertexID, vertexID);

    // walk through the vertex' edges
    for (vtkm::Id edge = 0; edge < outdegree.Get(vertexID); edge++)
    {
      vtkm::Id edgeIndex = edgeFirst + edge;
      vtkm::Id edgeID = activeEdges.Get(edgeIndex);

      // retrieve the vertex ID for the high end & update for pruning
      vtkm::Id highEnd = prunesTo.Get(chainExtremum.Get(edgeFar.Get(edgeID)));

      // we want to ignore edges that lead back to this vertex
      if (highEnd == vertexID)
        continue;

      // if we survived, increment the outdegree
      newOutdegree++;
    } // per edge
    return newOutdegree;
  }
}; // VertexDegreeUpdater

}
}
}

#endif
