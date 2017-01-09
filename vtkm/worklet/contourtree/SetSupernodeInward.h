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
// SetSupernodeInward.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_set_supernode_inward_h
#define vtkm_worklet_contourtree_set_supernode_inward_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Types.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for doing regular to candidate
class SetSupernodeInward : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> superID,            // (input) index into supernodes
                                WholeArrayIn<IdType> inbound,       // (input) join or split arc
                                WholeArrayIn<IdType> outbound,      // (input) join or split arc
                                WholeArrayIn<IdType> indegree,      // (input)
                                WholeArrayIn<IdType> outdegree,     // (input)
                                WholeArrayInOut<IdType> superarcs); // (in out)
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  SetSupernodeInward() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& superID,
                  const InFieldPortalType& inbound,
                  const InFieldPortalType& outbound,
                  const InFieldPortalType& indegree,
                  const InFieldPortalType& outdegree,
                  const OutFieldPortalType& superarcs) const
  {
    // test for criticality
    vtkm::Id outNeighbour = outbound.Get(superID);
    vtkm::Id inNeighbour = inbound.Get(superID);
    if (outNeighbour == NO_VERTEX_ASSIGNED)
      return;

    // test for leaf-ness
    if ((outdegree.Get(outNeighbour) != 0) || (indegree.Get(outNeighbour) != 1))
      return;

    // skip if the superarc is already set
    if (superarcs.Get(superID) != NO_VERTEX_ASSIGNED)
      return;

    // we've passed the tests - set the supernode to point inwards
    superarcs.Set(superID, inNeighbour);
  }
}; // SetSupernodeInward

}
}
}

#endif
