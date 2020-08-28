//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_connectivity_ImageConnectivity_h
#define vtk_m_worklet_connectivity_ImageConnectivity_h

#include <vtkm/cont/Invoker.h>
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

class ImageGraft : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn,
                                FieldInNeighborhood neighborComp,
                                FieldInNeighborhood neighborColor,
                                AtomicArrayInOut compOut);

  using ExecutionSignature = void(Boundary, _2, _3, _4);


  // compOut is a "linear" alias of neightborComp such that we can update component labels
  template <typename Boundary, typename NeighborComp, typename NeighborColor, typename CompOut>
  VTKM_EXEC void operator()(Boundary boundary,
                            const NeighborComp& neighborComp,
                            const NeighborColor& neighborColor,
                            CompOut& compOut) const
  {
    auto thisColor = neighborColor.Get(0, 0, 0);

    auto minIndices = boundary.MinNeighborIndices(1);
    auto maxIndices = boundary.MaxNeighborIndices(1);

    for (int k = minIndices[2]; k <= maxIndices[2]; k++)
    {
      for (int j = minIndices[1]; j <= maxIndices[1]; j++)
      {
        for (int i = minIndices[0]; i <= maxIndices[0]; i++)
        {
          // We need to reload thisComp and thatComp every iteration since
          // they might have been changed by Unite(), both as a result of
          // attaching one tree to the other or as a result of path compaction
          // in findRoot().
          auto thisComp = neighborComp.Get(0, 0, 0);
          auto thatComp = neighborComp.Get(i, j, k);
          if (thisColor == neighborColor.Get(i, j, k))
          {
            // Merge the two components one way or the other, the order will
            // be resolved by Unite().
            UnionFind::Unite(compOut, thisComp, thatComp);
          }
        }
      }
    }
  }
};
}

// Single pass connected component algorithm from
// Jaiganesh, Jayadharini, and Martin Burtscher.
// "A high-performance connected components implementation for GPUs."
// Proceedings of the 27th International Symposium on High-Performance
// Parallel and Distributed Computing. 2018.
class ImageConnectivity
{
public:
  class RunImpl
  {
  public:
    template <int Dimension, typename T, typename StorageT, typename OutputPortalType>
    void operator()(const vtkm::cont::ArrayHandle<T, StorageT>& pixels,
                    const vtkm::cont::CellSetStructured<Dimension>& input,
                    OutputPortalType& components) const
    {
      using Algorithm = vtkm::cont::Algorithm;

      // Initialize the parent pointer to point it the pixel itself. There are other
      // ways to initialize the parent points, for example, a smaller or the minimal
      // neighbor. This can potentially reduce the number of iteration by 1.
      Algorithm::Copy(vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, pixels.GetNumberOfValues()),
                      components);

      // TODO: give the reason that single pass algorithm works.
      vtkm::cont::Invoker invoke;
      invoke(detail::ImageGraft{}, input, components, pixels, components);
      invoke(PointerJumping{}, components);

      // renumber connected component to the range of [0, number of components).
      // FIXME: we should able to apply findRoot to each pixel and use some kind
      // of atomic operation to get the number of unique components without the
      // cost of copying and sorting. This might be able to be extended to also
      // work for the renumbering (replacing InnerJoin) through atomic increment.
      vtkm::cont::ArrayHandle<vtkm::Id> uniqueComponents;
      Algorithm::Copy(components, uniqueComponents);
      Algorithm::Sort(uniqueComponents);
      Algorithm::Unique(uniqueComponents);

      vtkm::cont::ArrayHandle<vtkm::Id> pixelIds;
      Algorithm::Copy(vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, pixels.GetNumberOfValues()),
                      pixelIds);

      vtkm::cont::ArrayHandle<vtkm::Id> uniqueColor;
      Algorithm::Copy(
        vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, uniqueComponents.GetNumberOfValues()),
        uniqueColor);

      vtkm::cont::ArrayHandle<vtkm::Id> cellColors;
      vtkm::cont::ArrayHandle<vtkm::Id> pixelIdsOut;
      InnerJoin().Run(
        components, pixelIds, uniqueComponents, uniqueColor, cellColors, pixelIdsOut, components);

      Algorithm::SortByKey(pixelIdsOut, components);
    }
  };

  class ResolveDynamicCellSet
  {
  public:
    template <int Dimension, typename T, typename StorageT, typename OutputPortalType>
    void operator()(const vtkm::cont::CellSetStructured<Dimension>& input,
                    const vtkm::cont::ArrayHandle<T, StorageT>& pixels,
                    OutputPortalType& components) const
    {
      vtkm::cont::CastAndCall(pixels, RunImpl(), input, components);
    }
  };

  template <int Dimension, typename T, typename OutputPortalType>
  void Run(const vtkm::cont::CellSetStructured<Dimension>& input,
           const vtkm::cont::VariantArrayHandleBase<T>& pixels,
           OutputPortalType& componentsOut) const
  {
    using Types = vtkm::TypeListScalarAll;
    vtkm::cont::CastAndCall(pixels.ResetTypes(Types{}), RunImpl(), input, componentsOut);
  }

  template <int Dimension, typename T, typename S, typename OutputPortalType>
  void Run(const vtkm::cont::CellSetStructured<Dimension>& input,
           const vtkm::cont::ArrayHandle<T, S>& pixels,
           OutputPortalType& componentsOut) const
  {
    vtkm::cont::CastAndCall(pixels, RunImpl(), input, componentsOut);
  }

  template <typename CellSetTag, typename T, typename S, typename OutputPortalType>
  void Run(const vtkm::cont::DynamicCellSetBase<CellSetTag>& input,
           const vtkm::cont::ArrayHandle<T, S>& pixels,
           OutputPortalType& componentsOut) const
  {
    input.ResetCellSetList(vtkm::cont::CellSetListStructured())
      .CastAndCall(ResolveDynamicCellSet(), pixels, componentsOut);
  }
};
}
}
}

#endif // vtk_m_worklet_connectivity_ImageConnectivity_h
