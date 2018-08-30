//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#ifndef vtk_m_worklet_connectivity_graph_connectivity_h
#define vtk_m_worklet_connectivity_graph_connectivity_h

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
  using ControlSignature = void(FieldIn<IdType> index,
                                FieldIn<IdType> start,
                                FieldIn<IdType> degree,
                                WholeArrayIn<IdType> ids,
                                WholeArrayInOut<IdType> comp);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);

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

template <typename DeviceAdapter>
class GraphConnectivity
{
public:
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  template <typename InputPortalType, typename OutputPortalType>
  void Run(const InputPortalType& numIndexArray,
           const InputPortalType& indexOffsetArray,
           const InputPortalType& connectivityArray,
           OutputPortalType& componentsOut) const
  {
    bool allStar = false;
    vtkm::cont::ArrayHandle<vtkm::Id> components;
    vtkm::cont::ArrayHandle<bool> isStar;
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
    Algorithm::Copy(
      vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, numIndexArray.GetNumberOfValues()), cellIds);
    Algorithm::Copy(cellIds, components);

    do
    {
      vtkm::worklet::DispatcherMapField<detail::Graft> graftDispatcher;
      graftDispatcher.SetDevice(DeviceAdapter());
      graftDispatcher.Invoke(
        cellIds, indexOffsetArray, numIndexArray, connectivityArray, components);

      // Detection of allStar has to come before pointer jumping. Don't try to rearrange it.
      vtkm::worklet::DispatcherMapField<IsStar> isStarDisp;
      isStarDisp.SetDevice(DeviceAdapter());
      isStarDisp.Invoke(cellIds, components, isStar);
      allStar = Algorithm::Reduce(isStar, true, vtkm::LogicalAnd());

      vtkm::worklet::DispatcherMapField<PointerJumping> pointJumpingDispatcher;
      pointJumpingDispatcher.SetDevice(DeviceAdapter());
      pointJumpingDispatcher.Invoke(cellIds, components);
    } while (!allStar);

    // renumber connected component to the range of [0, number of components).
    vtkm::cont::ArrayHandle<vtkm::Id> uniqueComponents;
    Algorithm::Copy(components, uniqueComponents);
    Algorithm::Sort(uniqueComponents);
    Algorithm::Unique(uniqueComponents);

    vtkm::cont::ArrayHandle<vtkm::Id> uniqueColor;
    Algorithm::Copy(
      vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, uniqueComponents.GetNumberOfValues()),
      uniqueColor);
    vtkm::cont::ArrayHandle<vtkm::Id> cellColors;
    vtkm::cont::ArrayHandle<vtkm::Id> cellIdsOut;
    InnerJoin<DeviceAdapter>().Run(
      components, cellIds, uniqueComponents, uniqueColor, cellColors, cellIdsOut, componentsOut);

    Algorithm::SortByKey(cellIdsOut, componentsOut);
  }
};
}
}
}
#endif //vtk_m_worklet_connectivity_graph_connectivity_h
