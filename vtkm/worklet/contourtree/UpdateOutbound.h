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

#ifndef vtkm_worklet_contourtree_update_outbound_h
#define vtkm_worklet_contourtree_update_outbound_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Types.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for doing regular to candidate
class UpdateOutbound : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> superID,           // input
                                WholeArrayInOut<IdType> outbound); // i/o
  typedef void ExecutionSignature(_1, _2);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  UpdateOutbound() {}

  template <typename InOutPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id &superID,
                  const InOutPortalType& outbound) const
  {
    vtkm::Id outNeighbour = outbound.Get(superID);

    // ignore if it has no out neighbour
    if (outNeighbour == NO_VERTEX_ASSIGNED)
      return;

    // if it's out neighbour has none itself, it's a critical point & we stop
    vtkm::Id doubleOut = outbound.Get(outNeighbour);
    if (doubleOut == NO_VERTEX_ASSIGNED)
      return;

    // otherwise, we update
    outbound.Set(superID, doubleOut);
  }
}; // UpdateOutbound

}
}
}

#endif
