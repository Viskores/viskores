//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_connectivity_union_find_h
#define vtk_m_worklet_connectivity_union_find_h

#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace connectivity
{

// Reference:
//     Jayanti, Siddhartha V., and Robert E. Tarjan.
//     "Concurrent Disjoint Set Union." arXiv preprint arXiv:2003.01203 (2020).
class PointerJumping : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayInOut comp);
  using ExecutionSignature = void(WorkIndex, _1);
  using InputDomain = _1;

  // This is the naive find() without path compaction in SV Jayanti et. al.
  // Since the comp array is read-only there is no data race. However, this
  // also makes the algorithm to have O(n) depth with O(n^2) of total work.
  template <typename Comp>
  VTKM_EXEC vtkm::Id findRoot(const Comp& comp, vtkm::Id index) const
  {
    while (comp.Get(index) != index)
      index = comp.Get(index);
    return index;
  }

  // This the find() with path compression. This guarantees that the output
  // trees will be rooted stars, i.e. they all have depth of 1.
  //
  // There is a "seemly" data race
  // between concurrent invocations of this operator(). The "root" returned
  // by findRoot() in one invocation might become out of date if some other
  // invocations change it while calling comp.Set(). However, the monotone
  // nature of the data structure and findRoot() makes it harmless as long as
  // the root of the tree does not change (such as by Union())

  // There is a data race between this operator() and some form of Union(), which
  // update the parent point of the root and make its value out of date.
  // TODO: is the data race harmful? can we do something about it?
  template <typename InOutPortalType>
  VTKM_EXEC void operator()(vtkm::Id index, InOutPortalType& comp) const
  {
    // There a data race between findRoot and comp.Set.
    auto root = findRoot(comp, index);
    comp.Set(index, root);
  }
};

class IsStar : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn comp, AtomicArrayInOut);
  using ExecutionSignature = void(WorkIndex, _1, _2);
  using InputDomain = _1;

  template <typename InPortalType, typename AtomicInOut>
  VTKM_EXEC void operator()(vtkm::Id index, const InPortalType& comp, AtomicInOut& hasStar) const
  {
    //hasStar emulates a LogicalAnd across all the values
    //where we start with a value of 'true'|1.
    // Note: comp.Get(index) == comp.Get(comp.Get(index)) applies for both the
    // root of the tree and the first level vertices. If all vertices
    // is either a root or first level vertices, it is a rooted star.
    const bool isAStar = (comp.Get(index) == comp.Get(comp.Get(index)));
    if (!isAStar && hasStar.Get(0) == 1)
    {
      hasStar.Set(0, 0);
    }
  }
};

} // connectivity
} // worklet
} // vtkm
#endif // vtk_m_worklet_connectivity_union_find_h
