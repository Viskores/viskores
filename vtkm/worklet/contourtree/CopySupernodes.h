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
// CopySupernodes.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_copy_supernodes_h
#define vtkm_worklet_contourtree_copy_supernodes_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for doing regular to candidate
class CopySupernodes : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> isSupernode,             // (input) is this a supernode
                                FieldIn<IdType> regularID,               // (input) candidate ID
                                FieldIn<IdType> superID,                 // (input) supernode ID
                                FieldIn<IdType> upCandidate,             // (input) 
                                FieldIn<IdType> downCandidate,           // (input)
                                WholeArrayOut<IdType> regularToCritical, // (output)
                                WholeArrayOut<IdType> supernodes,        // (output) compacted supernodes
                                WholeArrayOut<IdType> updegree,          // (output) compacted updegree
                                WholeArrayOut<IdType> downdegree);       // (output) compacted downdegree
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8, _9);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  CopySupernodes() {}

  template <typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& isSupernode,
                  const vtkm::Id& regularID,
                  const vtkm::Id& superID,
                  const vtkm::Id& upCandidate,
                  const vtkm::Id& downCandidate,
                  const OutFieldPortalType& regularToCritical,
                  const OutFieldPortalType& supernodes,
                  const OutFieldPortalType& updegree,
                  const OutFieldPortalType& downdegree) const
  {
    if (isSupernode)
    { // supernode
      // update the inverse lookup, &c.
      regularToCritical.Set(regularID, superID);
      supernodes.Set(superID, regularID);
      updegree.Set(superID, upCandidate);
      downdegree.Set(superID, downCandidate);
    }
  }
}; // CopySupernodes

}
}
}

#endif
