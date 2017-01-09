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
// ChainDoubler.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
// This functor implements chain-doubling (pointer-doubling), but minimises memory writeback
// by testing whether we've hit the end of the chain already
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_chain_doubler_h
#define vtkm_worklet_contourtree_chain_doubler_h

namespace vtkm {
namespace worklet {
namespace contourtree {

// Functor for doing chain doubling
// Unary because it takes the index of the element to process, and is not guaranteed to
// write back
// moreover, we aren't worried about out-of-sequence writes, since the worst that happens
// is that an element gets pointer-tripled in the iteration. It will still converge to the
// same destination.
class ChainDoubler : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertexID,
                                WholeArrayInOut<IdType> chains);
  typedef void ExecutionSignature(_1, _2);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  ChainDoubler() {}

  template <typename InOutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& vertexID,
                  const InOutFieldPortalType& chains) const
  {
    vtkm::Id next = chains.Get(vertexID);
    vtkm::Id doubleNext = chains.Get(next);

    if (next != doubleNext)
      chains.Set(vertexID, doubleNext);
  }
}; // ChainDoubler

}
}
}

#endif
