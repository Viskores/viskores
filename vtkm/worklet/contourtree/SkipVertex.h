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
// SkipVertex.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_skip_vertex_h
#define vtkm_worklet_contourtree_skip_vertex_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Types.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for doing regular to candidate
class SkipVertex : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> superID,            // (input) index into supernodes
                                WholeArrayIn<IdType> superarcs,     // (input)
                                WholeArrayInOut<IdType> joinArcs,   // (i/o)
                                WholeArrayInOut<IdType> splitArcs); // (i/o)
  typedef void ExecutionSignature(_1, _2, _3, _4);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  SkipVertex() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& superID,
                  const InFieldPortalType& superarcs,
                  const OutFieldPortalType& joinArcs,
                  const OutFieldPortalType& splitArcs) const
  {
    //  retrieve it's join neighbour j
    vtkm::Id joinNeighbour = joinArcs.Get(superID);

    // if v has a join neighbour (i.e. j == -1) and j has a contour arc
    if ((joinNeighbour != NO_VERTEX_ASSIGNED) && (superarcs.Get(joinNeighbour) != NO_VERTEX_ASSIGNED))
      // reset the vertex' join neighbour
      joinArcs.Set(superID, joinArcs.Get(joinNeighbour));

    // retrieve it's split neighbour s
    vtkm::Id splitNeighbour = splitArcs.Get(superID);

    // if v has a split neighbour (i.e. s == -1) and s has a contour arc
    if ((splitNeighbour != NO_VERTEX_ASSIGNED) && (superarcs.Get(splitNeighbour) != NO_VERTEX_ASSIGNED))
      // reset the vertex' split neighbour
      splitArcs.Set(superID, splitArcs.Get(splitNeighbour));
  }
}; // SkipVertex

}
}
}

#endif
