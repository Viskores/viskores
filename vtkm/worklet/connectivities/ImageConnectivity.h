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
                                AtomicArrayInOut compOut,
                                AtomicArrayInOut changed);

  using ExecutionSignature = void(_2, _3, _4, _5);

  // This is the naive find() without path compaction in SV Jayanti et. al.
  // Since the parents array is read-only there is no data race.
  // TODO: Since parents is now an AtomicArray with certain memory consistency,
  // consider changing this to find with path compaction.
  template <typename Parents>
  VTKM_EXEC vtkm::Id findRoot(const Parents& parents, vtkm::Id index) const
  {
    while (parents.Get(index) != index)
      index = parents.Get(index);
    return index;
  }

  template <typename Comp>
  VTKM_EXEC void Unite(Comp& compOut, vtkm::Id u, vtkm::Id v) const
  {
    auto thisRoot = findRoot(compOut, u);
    auto thatRoot = findRoot(compOut, v);

    // This is "linking by index" as in SV Jayanti et.al. with less than as the total
    // order. This avoids cycles in the resulting graph and maintains the rooted forest
    // structure of UnionFind. It is possible for two threads to try to change the
    // same old root to different new roots, e.g. threadA calls compOut.Set(root, rootB)
    // while threadB calls compOut(root, rootB) where rootB < root and rootC < root (but
    // the order of rootA and rootB is unspecified) and each thread assuming success
    // while the outcome is actually unspecified. An atomic Compare and Swap is suggested in
    // SV Janati et. al. to "resolve" data race. However, I don't see any
    // need to use CAS, it looks like the data race will always correct itself by the
    // algorithm if atomic Store of memory_order_release and Load of memory_order_acquire
    // is used (as provided by AtomicArrayInOut).
    if (thisRoot < thatRoot)
      compOut.Set(thatRoot, thisRoot);
    else if (thisRoot > thatRoot)
      compOut.Set(thisRoot, thatRoot);
    // else, no need to do anything when they are the same set.
  }

  // compOut is an alias of neightborComp such that we can update component labels
  template <typename NeighborComp, typename NeighborColor, typename CompOut, typename AtomicInOut>
  VTKM_EXEC void operator()(const NeighborComp& neighborComp,
                            const NeighborColor& neighborColor,
                            CompOut& compOut,
                            AtomicInOut& updated) const
  {
    vtkm::Id thisComp = neighborComp.Get(0, 0, 0);
    auto minComp = thisComp;
    auto thisColor = neighborColor.Get(0, 0, 0);

    // FIXME: we are doing this "local connectivity finding" at each call of this
    // worklet. This creates a large demand on the memory bandwidth.
    // Is this necessary? It looks like we only need a local, partial spanning
    // tree at the beginning. Is it true?
    for (int k = -1; k <= 1; k++)
    {
      for (int j = -1; j <= 1; j++)
      {
        for (int i = -1; i <= 1; i++)
        {
          if (thisColor == neighborColor.Get(i, j, k))
          {
            minComp = vtkm::Min(minComp, neighborComp.Get(i, j, k));
          }
        }
      }
    }

    // I don't want to just update the component label of this pixel to the next, I
    // actually want to merge the two gangs by Union(FindRoot(this), FindRoot(that))
    // and then Flatten the result.
    Unite(compOut, thisComp, minComp);

    // FIXME: is this the right termination condition?
    // FIXME: should the Get()/Set() be replaced with a CompareAnsSwap()?
    // mark an update occurred if no other worklets have done so yet
    if (thisComp != minComp && updated.Get(0) == 0)
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
        updateRequired.WritePortal().Set(0, 0); //reset the atomic state
        invoke(detail::ImageGraft{}, input, components, pixels, components, updateRequired);
        invoke(PointerJumping{}, components);
      } while (updateRequired.WritePortal().Get(0) > 0);

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
