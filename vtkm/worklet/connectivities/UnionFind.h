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
  // also makes the algorithm to have O(n) depth with O(n^2) of total work
  // on a Parallel Random Access Machine (PRAM). However, we don't live in
  // a synchronous, infinite number of processor PRAM world. In reality, since we put
  // "parent pointers" in a array and all the pointers are pointing from larger
  // index to smaller, invocation for
  // nodes with smaller ids are mostly likely be schedule before and complete
  // earlier than larger ids
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
  // There is a "seemly" data race between concurrent invocations of this
  // operator(). The "root" returned by findRoot() in one invocation might
  // become out of date if some other invocations change it while calling comp.Set(). However, the monotone
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

} // connectivity
} // worklet
} // vtkm
#endif // vtk_m_worklet_connectivity_union_find_h
