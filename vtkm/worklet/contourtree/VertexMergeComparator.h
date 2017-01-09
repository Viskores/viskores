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
// joinArcFunctor.cpp - functors for the thrust version of JoinTree.cpp
//
//=======================================================================================
//
// COMMENTS:
//
// Basically, we have a single functor (so far), whose job is to work out the downwards
// join neighbour of each vertex.
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_vertex_merge_comparator_h
#define vtkm_worklet_contourtree_vertex_merge_comparator_h

namespace vtkm {
namespace worklet {
namespace contourtree {

//=======================================================================================
//
//	VertexMergeComparator
//
// A comparator that sorts the vertices on the join maximum (assuming already sorted on
// indexed value)
// 
//=======================================================================================

template <typename T, typename StorageType, typename DeviceAdapter>
class VertexMergeComparator
{
public:
  typedef typename vtkm::cont::ArrayHandle<T,StorageType>::template ExecutionTypes<DeviceAdapter>::PortalConst ValuePortalType;
  typedef typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;

  ValuePortalType values;
  IdPortalType extrema;
  bool isJoinTree;

  VTKM_CONT
  VertexMergeComparator(ValuePortalType Values,
                        IdPortalType Extrema,
                        bool IsJoinTree)
		: values(Values), extrema(Extrema), isJoinTree(IsJoinTree)
		{}

  VTKM_EXEC_CONT
  bool operator() (const vtkm::Id& i, const vtkm::Id& j) const
  {
    // retrieve the pseudo-extremum the vertex belongs to
    vtkm::Id pseudoExtI = extrema.Get(i);
    vtkm::Id pseudoExtJ = extrema.Get(j);

    if (pseudoExtI < pseudoExtJ) 
      return false ^ isJoinTree;
    if (pseudoExtJ < pseudoExtI) 
      return true ^ isJoinTree;

    T valueI = values.Get(i);
    T valueJ = values.Get(j);

    if (valueI < valueJ) 
      return false ^ isJoinTree;
    if (valueI > valueJ) 
      return true ^ isJoinTree;
    if (i < j) 
      return false ^ isJoinTree;
    if (j < i) 
      return true ^ isJoinTree;
    return false; // true ^ isJoinTree;			
  }
}; // VertexMergeComparator

}
}
}

#endif
