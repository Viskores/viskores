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
// EdgePeakComparator.h - a comparator for edges that sorts on four indices
//
//=======================================================================================
//
// COMMENTS:
//
// A comparator that sorts edges by:
// i.   the chain maximum for the upper end of the edge
//          this clusters all edges together that lead to the chain maximum
// ii.  the index of the low end of the edge
//          this sorts the edges for the chain max by the low end
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_edge_peak_comparator_h
#define vtkm_worklet_contourtree_edge_peak_comparator_h

namespace vtkm {
namespace worklet {
namespace contourtree {

// Comparator for edges to sort governing saddles high
template <typename T, typename StorageType, typename DeviceAdapter>
class EdgePeakComparator
{
public:
  typedef typename vtkm::cont::ArrayHandle<T,StorageType>::template ExecutionTypes<DeviceAdapter>::PortalConst ValuePortalType;
  typedef typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;

  ValuePortalType values;
  IdPortalType valueIndex;
  IdPortalType edgeFar;
  IdPortalType edgeNear;
  IdPortalType arcArray;
  bool isJoinGraph;

  VTKM_CONT
  EdgePeakComparator(ValuePortalType Values,
                     IdPortalType ValueIndex,
                     IdPortalType EdgeFar,
                     IdPortalType EdgeNear,
                     IdPortalType ArcArray,
                     bool IsJoinGraph) : 
                               values(Values),
                               valueIndex(ValueIndex),
                               edgeFar(EdgeFar),
                               edgeNear(EdgeNear),
                               arcArray(ArcArray),
                               isJoinGraph(IsJoinGraph) {}

  VTKM_EXEC
  bool operator() (const vtkm::Id &i, const vtkm::Id &j) const
  {
    // first compare the far end
    if (edgeFar.Get(i) < edgeFar.Get(j))
      return true ^ isJoinGraph;
    if (edgeFar.Get(j) < edgeFar.Get(i))
      return false ^ isJoinGraph;

    // the compare the values of the low end
    vtkm::Id valueIndex1 = valueIndex.Get(edgeNear.Get(i));
    vtkm::Id valueIndex2 = valueIndex.Get(edgeNear.Get(j));

    if (values.Get(valueIndex1) < values.Get(valueIndex2))
      return true ^ isJoinGraph;
    if (values.Get(valueIndex2) < values.Get(valueIndex1))
      return false ^ isJoinGraph;

    // finally compare the indices
    if (valueIndex1 < valueIndex2)
      return true ^ isJoinGraph;
    if (valueIndex2 < valueIndex1)
      return false ^ isJoinGraph;

    if (i < j)
      return false ^ isJoinGraph;
    if (j < i)
      return true ^ isJoinGraph;

    // fallback can happen when multiple paths end at same extremum
    return false; //true ^ graph.isJoinGraph;		
  }

}; // EdgePeakComparator

}
}
}

#endif
