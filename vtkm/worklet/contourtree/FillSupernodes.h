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
// FillSupernodes.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_fill_supernodes_h
#define vtkm_worklet_contourtree_fill_supernodes_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet
class FillSupernodes : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> upCandidate,          // (input)
                                FieldIn<IdType> downCandidate,        // (input)
                                FieldOut<IdType> isSupernode);        // (output)
  typedef _3 ExecutionSignature(_1, _2);
  typedef _1 InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  FillSupernodes() {}

  vtkm::Id operator()(const vtkm::Id& upCandidate,
                      const vtkm::Id& downCandidate) const
  {
    vtkm::Id isSupernode = ((upCandidate != 1) || (downCandidate != 1));
    return isSupernode;
  }
}; // FillSupernodes

}
}
}

#endif
