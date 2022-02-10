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
#include <vtkm/filter/connected_components/worklet/CellSetDualGraph.h>
#include <vtkm/filter/connected_components/worklet/InnerJoin.h>
#include <vtkm/filter/connected_components/worklet/UnionFind.h>

namespace vtkm
{
namespace worklet
{
namespace connectivity
{
namespace detail
{
class GraphGraft : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn start,
                                FieldIn degree,
                                WholeArrayIn ids,
                                AtomicArrayInOut comp);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4);

  template <typename InPortalType, typename AtomicCompInOut>
  VTKM_EXEC void operator()(vtkm::Id index,
                            vtkm::Id start,
                            vtkm::Id degree,
                            const InPortalType& conn,
                            AtomicCompInOut& comp) const
  {
    for (vtkm::Id offset = start; offset < start + degree; offset++)
    {
      vtkm::Id neighbor = conn.Get(offset);

      // We need to reload thisComp and thatComp every iteration since
      // they might have been changed by Unite() both as a result of
      // attaching one tree to the other or as a result of path compression
      // in findRoot().
      auto thisComp = comp.Get(index);
      auto thatComp = comp.Get(neighbor);

      // Merge the two components one way or the other, the order will
      // be resolved by Unite().
      UnionFind::Unite(comp, thisComp, thatComp);
    }
  }
};
}

// Single pass connected component algorithm from
// Jaiganesh, Jayadharini, and Martin Burtscher.
// "A high-performance connected components implementation for GPUs."
// Proceedings of the 27th International Symposium on High-Performance
// Parallel and Distributed Computing. 2018.
class GraphConnectivity
{
public:
  template <typename InputArrayType, typename OutputArrayType>
  void Run(const InputArrayType& numIndicesArray,
           const InputArrayType& indexOffsetsArray,
           const InputArrayType& connectivityArray,
           OutputArrayType& componentsOut) const
  {
    VTKM_IS_ARRAY_HANDLE(InputArrayType);
    VTKM_IS_ARRAY_HANDLE(OutputArrayType);

    using Algorithm = vtkm::cont::Algorithm;

    // Initialize the parent pointer to point to the node itself. There are other
    // ways to initialize the parent pointers, for example, a smaller or the minimal
    // neighbor.
    Algorithm::Copy(vtkm::cont::ArrayHandleIndex(numIndicesArray.GetNumberOfValues()),
                    componentsOut);

    vtkm::cont::Invoker invoke;
    invoke(
      detail::GraphGraft{}, indexOffsetsArray, numIndicesArray, connectivityArray, componentsOut);
    invoke(PointerJumping{}, componentsOut);

    // renumber connected component to the range of [0, number of components).
    Renumber::Run(componentsOut);
  }
};
}
}
}
#endif //vtk_m_worklet_connectivity_graph_connectivity_h
