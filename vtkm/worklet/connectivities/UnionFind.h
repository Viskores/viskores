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
class UnionFind
{
public:
  // This is the naive findRoot() without path compaction in SV Jayanti et. al.
  // Since the parents array is read-only there is no data race, we can just
  // call Get() which actually calls Load() with memory_order_acquire ordering
  // which in turn ensure writes by other threads are reflected.
  template <typename Parents>
  static VTKM_EXEC vtkm::Id findRoot(const Parents& parents, vtkm::Id index)
  {
    while (parents.Get(index) != index)
      index = parents.Get(index);
    return index;
  }

  template <typename Parents>
  static VTKM_EXEC void Unite(Parents& parents, vtkm::Id u, vtkm::Id v)
  {
    // Data Race Resolutions
    // Since this function modifies the Union-Find data structure, concurrent
    // invocation of it by 2 or more threads causes potential data race. Here
    // is a case analysis why the potential data race does no harm in the
    // context of the single pass connected component algorithm.

    // Case 1, Two threads calling Unite(u, v) (and/or Unite(v, u)) concurrently.
    // Problem: One thread might attach u to v while the other thread attach
    // v to u, causing a cycle in the Union-Find data structure.
    // Resolution: This is not so much of a race condition but a problem with
    // the consistency of the algorithm. This can also happen in serial.
    // This is resolved by "linking by index" as in SV Jayanti et.al. with less
    // than as the total order. The two threads will make the same decision on
    // how to Unite the two trees (e.g. from root with larger id to root with
    // smaller id.) This avoids cycles in the resulting graph and maintains the
    // rooted forest structure of Union-Find at the expense of duplicated (but
    // benign) work.

    // Case 2, T0 calling Unite(u, v), T1 calling Unite(u, w) and T2 calling
    // Unite(v, s) concurrently.
    // Problem I: There is a potential write after read data race. After T0
    // calls findRoot for u and v, T1 might have called parents.Set(root_u, root_w)
    // thus changed root_u to root_w thus making root_u "obsolete" before T0 calls
    // parents.Set() on root_u/root_v.
    // When the root of the tree to be attached to (e.g. root_u, when root_u <  root_v)
    // is changed, there is no hazard, since we are just attaching a tree to a
    // now a non-root node, root_u, (thus, root_w <- root_u <- root_v) and three
    // components merged.
    // However, when the root of the attaching tree (root_v) is change, it
    // means that the root_u has been attached to yet some other root_s and became
    // a non-root node. If we are now attaching this non-root node to root_w we
    // would leave root_s behind and undoing previous work.
    // Resolution: TDB, mostly some reason for CompareAndSwap

    // Case 3. There is a potential concurrent write data race as it is possible
    // for two threads to try to change the same old root to different new roots,
    // e.g. threadA calls parents.Set(root, rootA) while threadB calls
    // parents(root, rootB) where rootB < root and rootA < root (but the order
    // of rootA and rootB is unspecified.) Each thread assumes success while
    // the outcome is actually unspecified.
    // Resolution: An atomic Compare and Swap is suggested in SV Janati et. al.
    // as well as J. Jaiganesht et. al. to resolve the data race. The CAS
    // checks if the old root is the same as what we expected. If so, there is
    // no data race, CAS will set the root to the desired value. The return value
    // from CAS will equal to our expected old root and signifies a successful
    // write which terminates the while loop.
    // If the old root is not what we expected, it was updated by some other
    // thread and the update by this thread fails. The root as updated by
    // the other thread is returned. This returned value would not equal to our desired new
    // root, signifying the need to retry with the while loop. On the other
    // hand,
    // Why simple Load/Store as provide by AtomicArray Get/Set do not work.

    auto root_u = UnionFind::findRoot(parents, u);
    auto root_v = UnionFind::findRoot(parents, v);

    while (root_u != root_v)
    {
      // FIXME: we might be executing the loop one extra time.
      // Nota Bene: VTKm's CompareAndSwap has a different order of parameters
      // than normal practice, it is (index, new, expected) rather than
      // (index, expected, new).
      if (root_u < root_v)
        root_v = parents.CompareAndSwap(root_v, root_u, root_v);
      else if (root_u > root_v)
        root_u = parents.CompareAndSwap(root_u, root_v, root_u);
    }
  }

  // This compresses the path from each node to its root thus flattening the
  // trees and guarantees that the output trees will be rooted stars, i.e.
  // they all have depth of 1.

  // There is a "seemly" data race for this function. The root returned by
  // findRoot() in one thread might become out of date if some other
  // thread changed it before this function calls parents.Set() making the
  // result tree not "short" enough and thus calls for a CompareAndSwap retry
  // loop. However, this data race does not happen for the following reasons:
  // 1. Since the only way for a root of a tree to be changed is through Unite(),
  // as long as there is no concurrent invocation of Unite() and Flatten() there
  // is no data race. This applies even for a compacting findRoot() which can
  // still only change the parents of non-root nodes.
  // 2. By the same token, since findRoot() does not change root and most
  // "damage" parents.Set() can do is resetting root's parent to itself,
  // the root of a tree can never be changed by this function. Thus, here
  // is no data race between concurrent invocations of this function.

  // Since the current findRoot() does not do path compaction, this algorithm
  // has O(n) depth with O(n^2) of total work on a Parallel Random Access
  // Machine (PRAM). However, we don't live in a synchronous, infinite number
  // of processor PRAM world. In reality, since we put "parent pointers" in a
  // array and all the pointers are pointing from larger indices to smaller
  // ones, invocation for nodes with smaller ids are mostly likely be scheduled
  // before and completes earlier than nodes with larger ids. This makes
  // the "effective" path length shorter for nodes with larger ids.
  // In this way, concurrency actually helps with algorithm complexity.
  template <typename Parents>
  static VTKM_EXEC void Flatten(Parents& parents, vtkm::Id index)
  {
    auto root = findRoot(parents, index);
    parents.Set(index, root);
  }
};

class PointerJumping : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayInOut comp);
  using ExecutionSignature = void(WorkIndex, _1);
  using InputDomain = _1;

  template <typename InOutPortalType>
  VTKM_EXEC void operator()(vtkm::Id index, InOutPortalType& comps) const
  {
    UnionFind::Flatten(comps, index);
  }
};

} // connectivity
} // worklet
} // vtkm
#endif // vtk_m_worklet_connectivity_union_find_h
