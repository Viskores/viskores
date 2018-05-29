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

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherPointNeighborhood.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>

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
template <int Dimension>
class ImageGraft;

template <>
class ImageGraft<2> : public vtkm::worklet::WorkletPointNeighborhood3x3x3
{
public:
  using ControlSignature = void(CellSetIn,
                                FieldInNeighborhood<> comp,
                                FieldInNeighborhood<> color,
                                FieldOut<> newComp);

  using ExecutionSignature = _4(_2, _3);

  template <typename Comp, typename NeighborColor>
  VTKM_EXEC vtkm::Id operator()(const Comp& comp, const NeighborColor& color) const
  {
    vtkm::Id myComp = comp.Get(0, 0, 0);
    auto myColor = color.Get(0, 0, 0);

    for (int j = -1; j <= 1; j++)
    {
      for (int i = -1; i <= 1; i++)
      {
        if (myColor == color.Get(i, j, 0))
        {
          myComp = vtkm::Min(myComp, comp.Get(i, j, 0));
        }
      }
    }
    return myComp;
  }
};
}

class ImageConnectivity
{
public:
  class RunImpl
  {
  public:
    template <typename StorageT, typename OutputPortalType, typename Device>
    void operator()(const vtkm::cont::ArrayHandle<vtkm::UInt8, StorageT>& pixels,
                    const vtkm::cont::CellSetStructured<2>& input,
                    OutputPortalType& componentsOut,
                    Device) const
    {
      using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

      // TODO: template pixel type?

      Algorithm::Copy(vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, pixels.GetNumberOfValues()),
                      componentsOut);

      vtkm::cont::ArrayHandle<vtkm::Id> newComponents;

      vtkm::cont::ArrayHandle<vtkm::Id> pixelIds;
      Algorithm::Copy(vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, pixels.GetNumberOfValues()),
                      pixelIds);

      bool allStar = false;
      vtkm::cont::ArrayHandle<bool> isStar;

      using DispatcherType =
        vtkm::worklet::DispatcherPointNeighborhood<detail::ImageGraft<2>, Device>;

      do
      {
        DispatcherType().Invoke(input, componentsOut, pixels, newComponents);

        // Detection of allStar has to come before pointer jumping. Don't try to rearrange it.
        vtkm::worklet::DispatcherMapField<IsStar, Device> isStarDisp;
        isStarDisp.Invoke(pixelIds, newComponents, isStar);
        allStar = Algorithm::Reduce(isStar, true, vtkm::LogicalAnd());

        vtkm::worklet::DispatcherMapField<PointerJumping, Device> pointJumpingDispatcher;
        pointJumpingDispatcher.Invoke(pixelIds, newComponents);

        Algorithm::Copy(newComponents, componentsOut);

      } while (!allStar);

      // renumber connected component to the range of [0, number of components).
      vtkm::cont::ArrayHandle<vtkm::Id> uniqueComponents;
      Algorithm::Copy(componentsOut, uniqueComponents);
      Algorithm::Sort(uniqueComponents);
      Algorithm::Unique(uniqueComponents);

      vtkm::cont::ArrayHandle<vtkm::Id> uniqueColor;
      Algorithm::Copy(
        vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, uniqueComponents.GetNumberOfValues()),
        uniqueColor);
      vtkm::cont::ArrayHandle<vtkm::Id> cellColors;
      vtkm::cont::ArrayHandle<vtkm::Id> pixelIdsOut;
      InnerJoin<Device>().Run(componentsOut,
                              pixelIds,
                              uniqueComponents,
                              uniqueColor,
                              cellColors,
                              pixelIdsOut,
                              componentsOut);

      Algorithm::SortByKey(pixelIdsOut, componentsOut);
    }
  };

  template <typename T, typename S, typename OutputPortalType, typename Device>
  void Run(const vtkm::cont::CellSetStructured<2>& input,
           const vtkm::cont::DynamicArrayHandleBase<T, S>& pixels,
           OutputPortalType& componentsOut,
           Device device) const
  {
    using Types = vtkm::ListTagBase<vtkm::UInt8>;
    vtkm::cont::CastAndCall(pixels.ResetTypeList(Types{}), RunImpl(), input, componentsOut, device);
  }

  template <typename T, typename S, typename OutputPortalType, typename Device>
  void Run(const vtkm::cont::CellSetStructured<2>& input,
           const vtkm::cont::ArrayHandle<T, S>& pixels,
           OutputPortalType& componentsOut,
           Device device) const
  {
    vtkm::cont::CastAndCall(pixels, RunImpl(), input, componentsOut, device);
  }
};
}
}
}

#endif
