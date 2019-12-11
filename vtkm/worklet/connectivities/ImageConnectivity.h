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
                                FieldInNeighborhood compIn,
                                FieldInNeighborhood color,
                                WholeArrayInOut compOut,
                                AtomicArrayInOut changed);

  using ExecutionSignature = void(WorkIndex, _2, _3, _4, _5);

  template <typename Comp>
  VTKM_EXEC vtkm::Id findRoot(Comp& comp, vtkm::Id index) const
  {
    while (comp.Get(index) != index)
      index = comp.Get(index);
    return index;
  }

  // compOut is an alias of compIn such that we can update component labels
  template <typename NeighborComp, typename NeighborColor, typename CompOut, typename AtomicInOut>
  VTKM_EXEC void operator()(const vtkm::Id index,
                            const NeighborComp& neighborComp,
                            const NeighborColor& neighborColor,
                            CompOut& compOut,
                            AtomicInOut& updated) const
  {
    vtkm::Id myComp = neighborComp.Get(0, 0, 0);
    auto minComp = myComp;

    auto myColor = neighborColor.Get(0, 0, 0);

    for (int k = -1; k <= 1; k++)
    {
      for (int j = -1; j <= 1; j++)
      {
        for (int i = -1; i <= 1; i++)
        {
          if (myColor == neighborColor.Get(i, j, k))
          {
            minComp = vtkm::Min(minComp, neighborComp.Get(i, j, k));
          }
        }
      }
    }
    // I don't just only want to update the component label of this pixel, I actually
    // want to Union(FindRoot(myComponent), FindRoot(minComp)) and then Flatten the
    // result.
    compOut.Set(index, minComp);

    auto myRoot = findRoot(compOut, myComp);
    auto newRoot = findRoot(compOut, minComp);

    if (myRoot < newRoot)
      compOut.Set(newRoot, myRoot);
    else if (myRoot > newRoot)
      compOut.Set(myRoot, newRoot);

    // mark an update occurred if no other worklets have done so yet
    if (myComp != minComp && updated.Get(0) == 0)
    {
      updated.Set(0, 1);
    }
  }
};
}

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

      Algorithm::Copy(vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, pixels.GetNumberOfValues()),
                      components);

      //used as an atomic bool, so we use Int32 as it the
      //smallest type that VTK-m supports as atomics
      vtkm::cont::ArrayHandle<vtkm::Int32> updateRequired;
      updateRequired.Allocate(1);

      vtkm::cont::Invoker invoke;
      do
      {
        updateRequired.GetPortalControl().Set(0, 0); //reset the atomic state
        invoke(detail::ImageGraft{}, input, components, pixels, components, updateRequired);
        invoke(PointerJumping{}, components);
      } while (updateRequired.GetPortalControl().Get(0) > 0);

      // renumber connected component to the range of [0, number of components).
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
