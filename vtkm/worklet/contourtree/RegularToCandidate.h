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
// RegularToCritical.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_regular_to_candidate_h
#define vtkm_worklet_contourtree_regular_to_candidate_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include "vtkm/worklet/contourtree/Types.h"

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for doing regular to candidate
class RegularToCandidate : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertexId,               // (input) vertex id of candidate
                                WholeArrayIn<IdType> mergeArcs,         // (input) merge arcs
                                WholeArrayIn<IdType> regularToCritical, // (input) sorting indices
                                FieldOut<IdType> sortVector);           // (output) target for write back
  typedef _4 ExecutionSignature(_1, _2, _3);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  RegularToCandidate() {}

  template <typename InFieldPortalType>
  VTKM_EXEC
  vtkm::Id operator()(const vtkm::Id& vertexID,
                      const InFieldPortalType& mergeArcs,
                      const InFieldPortalType& regularToCritical) const
  {
    vtkm::Id sortVector;

    // copy the mergeArc ID
    vtkm::Id joinNeighbour = mergeArcs.Get(vertexID);

    // if it's the root vertex
    if (joinNeighbour == NO_VERTEX_ASSIGNED)
      // set it to the sentinel value
      sortVector = NO_VERTEX_ASSIGNED;
    else
      // otherwise convert to a candidate ID & save
      sortVector = regularToCritical.Get(joinNeighbour);
    return sortVector;
  }
}; // RegularToCandidate

}
}
}

#endif
