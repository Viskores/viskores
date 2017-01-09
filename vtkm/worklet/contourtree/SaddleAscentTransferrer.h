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
// SaddleAscentTransferrer.h - functor that transfers active saddle edges
//
//=======================================================================================
//
// COMMENTS:
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_saddle_ascent_transferrer_h
#define vtkm_worklet_contourtree_saddle_ascent_transferrer_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for setting initial chain maximum value
class SaddleAscentTransferrer : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertexID,             // (input) active vertex
                                FieldIn<IdType> newOutdegree,         // (input) updated updegree
                                FieldIn<IdType> newFirstEdge,         // (input) updated first edge of vertex
                                WholeArrayIn<IdType> activeEdges,     // (input) active edges
                                WholeArrayIn<IdType> firstEdge,       // (input) first edges
                                WholeArrayOut<IdType> edgeSorter);    // (output) edge sorter
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  SaddleAscentTransferrer() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& vertexID,
                  const vtkm::Id& newOutdegree,
                  const vtkm::Id& newFirstEdge,
                  const InFieldPortalType& activeEdges,
                  const InFieldPortalType& firstEdge,
                  const OutFieldPortalType& edgeSorter) const
  {
    // loop through the edges from the vertex
    for (vtkm::Id edge = 0; edge < newOutdegree; edge++)
    {
      // compute which index in the new sorting array
      vtkm::Id edgeSorterIndex = newFirstEdge + edge;
      // now retrieve the old edge
      vtkm::Id edgeID = activeEdges.Get(firstEdge.Get(vertexID) + edge);
      // adding them to the edge sort array
      edgeSorter.Set(edgeSorterIndex, edgeID);
    }
  }
		
}; // SaddleAscentTransferrer

}
}
}

#endif
