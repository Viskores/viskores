//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_connectivity_graph_connectivity_h
#define vtk_m_worklet_connectivity_graph_connectivity_h

#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/connectivities/CellSetDualGraph.h>
#include <vtkm/worklet/connectivities/InnerJoin.h>
#include <vtkm/worklet/connectivities/UnionFind.h>

namespace vtkm
{
namespace worklet
{
namespace connectivity
{
namespace detail
{
class Graft : public vtkm::worklet::WorkletMapField
{
public:
  // TODO: make sure AtomicArrayInOut is absolutely necessary
  using ControlSignature = void(FieldIn start,
                                FieldIn degree,
                                WholeArrayIn ids,
                                WholeArrayInOut comp);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4);

  using InputDomain = _1;

  template <typename Parents>
  VTKM_EXEC vtkm::Id findRoot(const Parents& parents, vtkm::Id index) const
  {
    while (parents.Get(index) != index)
      index = parents.Get(index);
    return index;
  }

  template <typename Parents>
  VTKM_EXEC void Unite(Parents& parents, vtkm::Id u, vtkm::Id v) const
  {
    // Data Race Resolutions
    // Since this function modifies the Union-Find data structure, concurrent
    // invocation of it by 2 or more threads cause potential data race. Here
    // is a case analysis why the potential data race does no harm in the
    // context of the iterative connected component algorithm.

    // Case 1, Two threads calling Unite(u, v) concurrently.
    // Problem: One thread might attach u to v while the other thread attach
    // v to u, causing a cycle in the Union-Find data structure.
    // Resolution: This is not necessary a data race issue. This is resolved by
    // "linking by index" as in SV Jayanti et.al. with less than as the total order.
    // The two threads will make the same decision on how to Unite the two tree
    // (e.g. from root with larger id to root with smaller id.) This avoids cycles in
    // the resulting graph and maintains the rooted forest structure of Union-Find.

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
    // Resolution:
    auto root_u = findRoot(parents, u);
    auto root_v = findRoot(parents, v);

    // Case 3. There is a potential concurrent write data race as it is possible for
    // two threads to try to change the same old root to different new roots,
    // e.g. threadA calls parents.Set(root, rootB) while threadB calls
    // parents(root, rootB) where rootB < root and rootC < root (but the order
    // of rootA and rootB is unspecified.) Each thread assumes success while
    // the outcome is actually unspecified. An atomic Compare and Swap is
    // suggested in SV Janati et. al. to "resolve" data race. However, I don't
    // see any need to use CAS, it looks like the data race will always correct
    // itself by the algorithm in later iterations as long as atomic Store of
    // memory_order_release and Load of memory_order_acquire is used (as provided
    // by AtomicArrayInOut.) This memory consistency model is the default mode
    // for x86, thus having zero extra cost but might be required for CUDA and/or ARM.
    if (root_u < root_v)
      parents.Set(root_v, root_u);
    else if (root_u > root_v)
      parents.Set(root_u, root_v);
    // else, no need to do anything when they are the same set.
  }

  // TODO: Use Scatter?
  template <typename InPortalType, typename InOutPortalType>
  VTKM_EXEC void operator()(vtkm::Id index,
                            vtkm::Id start,
                            vtkm::Id degree,
                            const InPortalType& conn,
                            InOutPortalType& comp) const
  {
    for (vtkm::Id offset = start; offset < start + degree; offset++)
    {
      vtkm::Id neighbor = conn.Get(offset);
      // We need to reload thisComp and thatComp every iteration since
      // they might have been changed by Unite() both as a result of
      // attaching on tree to the other or as a result of path compression
      // in findRoot().
      auto thisComp = comp.Get(index);
      auto thatComp = comp.Get(neighbor);
      // We need to reload thisComp and thatComp every iteration since
      // they might be changed by Unite()
      Unite(comp, thisComp, thatComp);
    }
  }
};
}

class GraphConnectivity
{
public:
  using Algorithm = vtkm::cont::Algorithm;

  template <typename InputPortalType, typename OutputPortalType>
  void Run(const InputPortalType& numIndicesArray,
           const InputPortalType& indexOffsetsArray,
           const InputPortalType& connectivityArray,
           OutputPortalType& componentsOut) const
  {
    vtkm::cont::ArrayHandle<vtkm::Id> components;
    Algorithm::Copy(
      vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, numIndicesArray.GetNumberOfValues()),
      components);

    // TODO: give the reason that single pass algorithm works.
    vtkm::cont::Invoker invoke;
    invoke(detail::Graft{}, indexOffsetsArray, numIndicesArray, connectivityArray, components);
    invoke(PointerJumping{}, components);

    // renumber connected component to the range of [0, number of components).
    vtkm::cont::ArrayHandle<vtkm::Id> uniqueComponents;
    Algorithm::Copy(components, uniqueComponents);
    Algorithm::Sort(uniqueComponents);
    Algorithm::Unique(uniqueComponents);

    vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
    Algorithm::Copy(
      vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, numIndicesArray.GetNumberOfValues()),
      cellIds);

    vtkm::cont::ArrayHandle<vtkm::Id> uniqueColor;
    Algorithm::Copy(
      vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, uniqueComponents.GetNumberOfValues()),
      uniqueColor);
    vtkm::cont::ArrayHandle<vtkm::Id> cellColors;
    vtkm::cont::ArrayHandle<vtkm::Id> cellIdsOut;
    InnerJoin().Run(
      components, cellIds, uniqueComponents, uniqueColor, cellColors, cellIdsOut, componentsOut);

    Algorithm::SortByKey(cellIdsOut, componentsOut);
  }
};
}
}
}
#endif //vtk_m_worklet_connectivity_graph_connectivity_h
