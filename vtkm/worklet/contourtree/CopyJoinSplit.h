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
// CopyJoinSplit.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_copy_join_split_h
#define vtkm_worklet_contourtree_copy_join_split_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Types.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for doing regular to candidate
class CopyJoinSplit : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> superID,             // (input) index into super nodes
                                WholeArrayIn<IdType> inbound,        // (input) join or split arcs
                                WholeArrayIn<IdType> indegree,       // (input)
                                WholeArrayIn<IdType> outdegree,      // (input)
                                WholeArrayOut<IdType> outbound);     // (output) join or split arcs
  typedef void ExecutionSignature(_1, _2, _3, _4, _5);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  CopyJoinSplit() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& superID,
                  const InFieldPortalType& inbound,
                  const InFieldPortalType& indegree,
                  const InFieldPortalType& outdegree,
                  const OutFieldPortalType& outbound) const
  {
    // if the vertex is critical, set it to -1
    if ((outdegree.Get(superID) != 1) || (indegree.Get(superID) != 1))
      outbound.Set(superID, NO_VERTEX_ASSIGNED);

    // check the inbound neighbour
    // if regular, set it to point outwards
    vtkm::Id inNeighbour = inbound.Get(superID);
    if (inNeighbour != NO_VERTEX_ASSIGNED)
    { // inbound exists
      if ((outdegree.Get(inNeighbour) == 1) && (indegree.Get(inNeighbour) == 1))
        outbound.Set(inNeighbour, superID);
    } // inbound exists
  }
}; // CopyJoinSplit

}
}
}

#endif
