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
#include <vtkm/cont/UncertainArrayHandle.h>
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
  template <typename Boundary,
            typename NeighborComp,
            typename NeighborColor,
            typename AtomicCompOut>
  VTKM_EXEC void operator()(Boundary boundary,
                            const NeighborComp& neighborComp,
                            const NeighborColor& neighborColor,
                            AtomicCompOut& compOut) const
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
          if (thisColor == neighborColor.Get(i, j, k))
          {
            // We need to reload thisComp and thatComp every iteration since
            // they might have been changed by Unite(), both as a result of
            // attaching one tree to the other or as a result of path compaction
            // in findRoot().
            auto thisComp = neighborComp.Get(0, 0, 0);
            auto thatComp = neighborComp.Get(i, j, k);

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
                    OutputPortalType& componentsOut) const
    {
      using Algorithm = vtkm::cont::Algorithm;

      // Initialize the parent pointer to point to the pixel itself. There are other
      // ways to initialize the parent pointers, for example, a smaller or the minimal
      // neighbor.
      Algorithm::Copy(vtkm::cont::ArrayHandleIndex(pixels.GetNumberOfValues()), componentsOut);

      vtkm::cont::Invoker invoke;
      invoke(detail::ImageGraft{}, input, componentsOut, pixels, componentsOut);
      invoke(PointerJumping{}, componentsOut);

      // renumber connected component to the range of [0, number of components).
      Renumber::Run(componentsOut);
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

  template <int Dimension, typename OutputPortalType>
  void Run(const vtkm::cont::CellSetStructured<Dimension>& input,
           const vtkm::cont::UnknownArrayHandle& pixels,
           OutputPortalType& componentsOut) const
  {
    using Types = vtkm::TypeListScalarAll;
    using Storages = VTKM_DEFAULT_STORAGE_LIST;
    vtkm::cont::CastAndCall(pixels.ResetTypes<Types, Storages>(), RunImpl(), input, componentsOut);
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
