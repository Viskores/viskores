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
  using ControlSignature = void(FieldIn start,
                                FieldIn degree,
                                WholeArrayIn ids,
                                WholeArrayInOut comp);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4);

  using InputDomain = _1;

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
      if ((comp.Get(index) == comp.Get(comp.Get(index))) && (comp.Get(neighbor) < comp.Get(index)))
      {
        comp.Set(comp.Get(index), comp.Get(neighbor));
      }
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
    bool everythingIsAStar = false;
    vtkm::cont::ArrayHandle<vtkm::Id> components;
    Algorithm::Copy(
      vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, numIndicesArray.GetNumberOfValues()),
      components);

    //used as an atomic bool, so we use Int32 as it the
    //smallest type that VTK-m supports as atomics
    vtkm::cont::ArrayHandle<vtkm::Int32> allStars;
    allStars.Allocate(1);

    vtkm::cont::Invoker invoke;

    do
    {
      allStars.GetPortalControl().Set(0, 1); //reset the atomic state
      invoke(detail::Graft{}, indexOffsetsArray, numIndicesArray, connectivityArray, components);

      // Detection of allStars has to come before pointer jumping. Don't try to rearrange it.
      invoke(IsStar{}, components, allStars);
      everythingIsAStar = (allStars.GetPortalControl().Get(0) == 1);
      invoke(PointerJumping{}, components);
    } while (!everythingIsAStar);

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
