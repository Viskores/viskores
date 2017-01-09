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
// ActiveEdgeTransferrer.h - functor that sets new active edges per vertex
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

#ifndef vtk_m_worklet_contourtree_active_edge_transferrer_h
#define vtk_m_worklet_contourtree_active_edge_transferrer_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet: set initial chain maximum value
template <typename DeviceAdapter>
class ActiveEdgeTransferrer : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertexID,                // (input) active vertex ID
                                FieldIn<IdType> newPosition,             // (input) new position of edge in array
                                FieldIn<IdType> newOutdegree,            // (input) the new updegree computed
                                WholeArrayInOut<IdType> firstEdge,       // (i/o) first edge of each active vertex
                                WholeArrayInOut<IdType> outdegree,       // (i/o) existing vertex updegrees
                                WholeArrayInOut<IdType> chainExtremum,   // (i/o) chain extremum for vertices
                                WholeArrayInOut<IdType> edgeFar,         // (i/o) high end of each edge
                                WholeArrayOut<IdType> newActiveEdges);   // (output) new active edge list
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  typedef _1   InputDomain;

  // Passed in constructor because of argument limit on operator
  typedef typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;

  IdPortalType activeEdges;   // (input) active edges
  IdPortalType prunesTo;      // (input) where a vertex prunes to

  // Constructor
  VTKM_EXEC_CONT
  ActiveEdgeTransferrer(IdPortalType ActiveEdges,
                        IdPortalType PrunesTo) :
                          activeEdges(ActiveEdges),
                          prunesTo(PrunesTo) {}

  // WARNING: POTENTIAL RISK FOR I/O
  // chainMaximum is safe for I/O here because:
  // 		we have previously eliminated maxima from the active vertex list
  //		we lookup chainMaximum of edgeHigh, which is guaranteed to be a maximum
  //		therefore, the chainMaximum entries edited are *NEVER* also accessed & v.v.
  //		edgeHigh is safe to edit, because each element only accesses it's own, and
  //		reads the current value before writing to it
  //		the same is true of firstEdge and updegree

  template <typename InOutFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id &vertexID,
                  const vtkm::Id &newPosition,
                  const vtkm::Id &newOutdegree,
                  const InOutFieldPortalType &firstEdge,
                  const InOutFieldPortalType &outdegree,
                  const InOutFieldPortalType &chainExtremum,
                  const InOutFieldPortalType &edgeFar,
                  const OutFieldPortalType &newActiveEdges) const
  {
    // retrieve actual vertex ID & first edge
    vtkm::Id edgeFirst = firstEdge.Get(vertexID);

    // internal counter for # of edges
    vtkm::Id whichEdge = newPosition;

    // walk through the vertex edges, counting as we go
    for (vtkm::Id edge = 0; edge < outdegree.Get(vertexID); edge++)
    {
      // compute the index and edge ID of this edge
      vtkm::Id edgeIndex = edgeFirst + edge;
      vtkm::Id edgeID = activeEdges.Get(edgeIndex);

      // retrieve the vertex ID for the high end & update for pruning
      vtkm::Id highEnd = prunesTo.Get(chainExtremum.Get(edgeFar.Get(edgeID)));

      // we want to ignore edges that lead back to this vertex
      if (highEnd != vertexID)
      {
        // reset the high end of the edge, copying downwards
        edgeFar.Set(edgeID, highEnd);

        // and keep the edge around
        newActiveEdges.Set(whichEdge++, edgeID);

        // and reset the chain maximum for good measure
        chainExtremum.Set(vertexID, highEnd);
      }
    }

    // now reset the firstEdge variable for this vertex
    outdegree.Set(vertexID, newOutdegree);
    firstEdge.Set(vertexID, newPosition);
  }
}; // ActiveEdgeTransferrer

}
}
}

#endif
