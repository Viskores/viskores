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
// GoverningSaddleFinder.h - iterator for finding governing saddle
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

#ifndef vtkm_worklet_contourtree_governing_saddle_finder_h
#define vtkm_worklet_contourtree_governing_saddle_finder_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for setting initial chain maximum value
class GoverningSaddleFinder : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> edgeNo,           // (input) index into sorted edges
                                WholeArrayIn<IdType> edgeSorter,  // (input) sorted edge index
                                WholeArrayIn<IdType> edgeFar,     // (input) high ends of edges
                                WholeArrayIn<IdType> edgeNear,    // (input) low ends of edges
                                WholeArrayOut<IdType> prunesTo,   // (output) where vertex is pruned to
                                WholeArrayOut<IdType> outdegree); // (output) updegree of vertex
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  GoverningSaddleFinder() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id &edgeNo,
                  const InFieldPortalType &edgeSorter,
                  const InFieldPortalType &edgeFar,
                  const InFieldPortalType &edgeNear,
                  const OutFieldPortalType &prunesTo,
                  const OutFieldPortalType &outdegree) const
  {
    // default to true        
    bool isBestSaddleEdge = true;

    // retrieve the edge ID
    vtkm::Id edge = edgeSorter.Get(edgeNo);
                
    // edge no. 0 is always best, so skip it
    if (edgeNo != 0)
    {
      // retrieve the previous edge
      vtkm::Id prevEdge = edgeSorter.Get(edgeNo-1);
      // if the previous edge has the same far end
      if (edgeFar.Get(prevEdge) == edgeFar.Get(edge))
        isBestSaddleEdge = false;
    }
                
    if (isBestSaddleEdge)
    { // found an extremum
      // retrieve the near end as the saddle
      vtkm::Id saddle = edgeNear.Get(edge);
      // and the far end as the extremum
      vtkm::Id extreme = edgeFar.Get(edge);

      // set the extremum to point to the saddle in the chainExtremum array
      prunesTo.Set(extreme, saddle);
                        
      // and set the outdegree to 0
      outdegree.Set(extreme,  0);
    } // found a extremum
  } // operator()

}; // GoverningSaddleFinder

}
}
}

#endif
