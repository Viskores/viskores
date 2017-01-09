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
// JoinArcConnector.h - functor that sets the final join arc connections
//
//=======================================================================================
//
// COMMENTS:
//
// This functor checks the vertex next lowest in the sort order. If it shares a maximum,
// we connect to it, otherwise we connect to the maximum's saddle.
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_join_arc_connector_h
#define vtkm_worklet_contourtree_join_arc_connector_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for setting initial chain maximum value
class JoinArcConnector : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertex,            // (input) index into sorted edges
                                WholeArrayIn<IdType> vertexSorter, // (input) sorting indices
                                WholeArrayIn<IdType> extrema,      // (input) maxima
                                WholeArrayIn<IdType> saddles,      // (input) saddles
                                WholeArrayOut<IdType> mergeArcs);  // (output) target for write back
  typedef void ExecutionSignature(_1, _2, _3, _4, _5);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  JoinArcConnector() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& vertex,
                  const InFieldPortalType& vertexSorter,
                  const InFieldPortalType& extrema,
                  const InFieldPortalType& saddles,
                  const OutFieldPortalType& mergeArcs) const
  {
    // work out whether we have the low element on the join arc
    bool joinToSaddle = false;
    if (vertex == 0) {
      joinToSaddle = true;
    } else {
      joinToSaddle = (extrema.Get(vertexSorter.Get(vertex)) != extrema.Get(vertexSorter.Get(vertex-1))); 
    }
						
    // now set the join arcs for everybody
    if (joinToSaddle)
      mergeArcs.Set(vertexSorter.Get(vertex), saddles.Get(vertexSorter.Get(vertex)));
    else
      mergeArcs.Set(vertexSorter.Get(vertex), vertexSorter.Get(vertex - 1));
  }
}; // JoinArcConnector

}
}
}

#endif
