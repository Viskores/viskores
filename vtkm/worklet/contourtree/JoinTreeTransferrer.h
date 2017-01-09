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
// JoinTreeTransferrer.h - functor that sets new active edges per vertex
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

#ifndef vtkm_worklet_contourtree_join_tree_transferrer_h
#define vtkm_worklet_contourtree_join_tree_transferrer_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Types.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for setting initial chain maximum value
class JoinTreeTransferrer : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertex,             // (input) index into active vertices
                                FieldIn<IdType> prunesTo,           // (input) where vertex is pruned to
                                WholeArrayIn<IdType> valueIndex,    // (input) indices into main array
                                WholeArrayIn<IdType> chainExtemum,  // (input) chain extemum for vertices
                                WholeArrayOut<IdType> saddles,      // (output) saddle array for writing
                                WholeArrayOut<IdType> arcArray);    // (output) arc / max array for writing 
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  JoinTreeTransferrer() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& vertex,
                  const vtkm::Id& prunesTo,
                  const InFieldPortalType& valueIndex,
                  const InFieldPortalType& chainExtremum,
                  const OutFieldPortalType& saddles,
                  const OutFieldPortalType& arcArray) const
  {
    // convert vertex & prunesTo to indices in original data
    // and write to saddle array
    if (prunesTo == NO_VERTEX_ASSIGNED)
      saddles.Set(valueIndex.Get(vertex), NO_VERTEX_ASSIGNED);
    else
      saddles.Set(valueIndex.Get(vertex), valueIndex.Get(prunesTo));
    // and in either event, we need to transfer the chain maximum
    arcArray.Set(valueIndex.Get(vertex), valueIndex.Get(chainExtremum.Get(vertex)));
    }
}; // JoinTreeTransferrer

}
}
}

#endif
