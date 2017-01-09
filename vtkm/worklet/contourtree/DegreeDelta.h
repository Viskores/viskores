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
// DegreeDelta.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_degree_delta_h
#define vtkm_worklet_contourtree_degree_delta_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for doing regular to candidate
class DegreeDelta : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> sortID,               // (input) index into sorted vertices [1..N]
                                WholeArrayIn<IdType> sortVector,      // (input) sorted vector of vertices
                                WholeArrayOut<IdType> candidate);     // (output) candidate
  typedef void ExecutionSignature(_1, _2, _3);
  typedef _1   InputDomain;

  vtkm::Id nCandidates;

  // Constructor
  VTKM_EXEC_CONT
  DegreeDelta(vtkm::Id NCandidates) : nCandidates(NCandidates) {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& sortID,
                  const InFieldPortalType& sortVector,
                  const OutFieldPortalType& candidate) const
  {
    vtkm::Id iCandidate = sortVector.Get(sortID);
    // last element needs to be subtracted from vector size
    if (sortID == nCandidates-1)
    { // final element
      candidate.Set(iCandidate, (nCandidates - candidate.Get(iCandidate)));
    } // final element
    // otherwise, if the next one is different
    else
    { // not the last element
      vtkm::Id nextCandidate = sortVector.Get(sortID+1);
      if (iCandidate != nextCandidate)
      { // no match
        candidate.Set(iCandidate, (sortID+1 - candidate.Get(iCandidate)));
      } // no match                   
    } // not the last element
  }
}; // DegreeDelta

}
}
}

#endif
