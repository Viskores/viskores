//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_m_worklet_ExtractCells_h
#define vtkm_m_worklet_ExtractCells_h

#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/ImplicitFunctions.h>

namespace vtkm {
namespace worklet {

class ExtractCells : public vtkm::worklet::WorkletMapPointToCell
{
public:
  ExtractCells() {}

  // Set mask for any cell whose points are inside volume of interest
  template<typename ImplicitFunction>
  class ExtractCellsWithImplicitFunction : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdComponentType> numIndices,
                                  FieldIn<IdType> indexOffset,
                                  WholeArrayIn<IdType> connectivity,
                                  WholeArrayIn<Vec3> coordinates,
                                  FieldOut<IdComponentType> mask);
    typedef   _5 ExecutionSignature(_1, _2, _3, _4);

    VTKM_CONT
    ExtractCellsWithImplicitFunction(const ImplicitFunction &function) : Function(function) {}

    template <typename InFieldPortalType, typename InVecFieldPortalType>
    VTKM_CONT
    vtkm::IdComponent operator()(const vtkm::IdComponent &numIndices,
                                 const vtkm::Id &indexOffset,
                                 const InFieldPortalType &connectivity,
                                 const InVecFieldPortalType &coordinates) const
    {
      // If any point is outside volume of interest, cell is also
      vtkm::IdComponent mask = 1;
      for (vtkm::Id indx = indexOffset; indx < (indexOffset+numIndices); indx++)
      {
        vtkm::Id ptId = connectivity.Get(indx);
        vtkm::Vec<FloatDefault,3> coordinate = coordinates.Get(ptId);
        vtkm::FloatDefault value = this->Function.Value(coordinate);
        if (value > 0)
          mask = 0;
      }
      return mask;
    }

  private:
    ImplicitFunction Function;
  };

  // Worklet to build new connectivity array from original cellset
  // Point ids are the same because usused points are not removed
  class BuildConnectivity : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdComponentType> numIndices,
                                  FieldIn<IdType> inIndexOffset,
                                  FieldIn<IdType> outIndexOffset,
                                  WholeArrayIn<IdType> inConnectivity,
                                  WholeArrayOut<IdType> outConnectivity);
    typedef void ExecutionSignature(_1, _2, _3, _4, _5);

    VTKM_CONT
    BuildConnectivity() {}

    template <typename InPortalType, typename OutPortalType>
    VTKM_CONT
    void operator()(const vtkm::IdComponent &numIndices,
                    const vtkm::Id &inIndexOffset,
                    const vtkm::Id &outIndexOffset,
                    const InPortalType &inConnectivity,
                          OutPortalType &outConnectivity) const
    {
      vtkm::Id inIndex = inIndexOffset; 
      vtkm::Id outIndex = outIndexOffset; 
      for (vtkm::IdComponent indx = 0; indx < numIndices; indx++)
      {
        outConnectivity.Set(outIndex++, inConnectivity.Get(inIndex++));
      }
    }
  };

  // Extract cells by id and building new connectivity using original point ids
  template <typename DeviceAdapter>
  vtkm::cont::CellSetExplicit<> DoExtract(const vtkm::cont::CellSetExplicit<> &cellSet,
                                          const vtkm::cont::ArrayHandle<vtkm::Id> &cellIds,
                                          DeviceAdapter device)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;

    vtkm::cont::ArrayHandlePermutation<vtkm::cont::ArrayHandle<vtkm::Id>,
                                       vtkm::cont::ArrayHandle<vtkm::UInt8> >
                     permuteShapes(cellIds, 
                                   cellSet.GetShapesArray(vtkm::TopologyElementTagPoint(), 
                                                          vtkm::TopologyElementTagCell()));
    vtkm::cont::ArrayHandlePermutation<vtkm::cont::ArrayHandle<vtkm::Id>,
                                       vtkm::cont::ArrayHandle<vtkm::IdComponent> >
                     permuteNumIndices(cellIds, 
                                       cellSet.GetNumIndicesArray(vtkm::TopologyElementTagPoint(),
                                                                  vtkm::TopologyElementTagCell()));
    vtkm::cont::ArrayHandlePermutation<vtkm::cont::ArrayHandle<vtkm::Id>,
                                       vtkm::cont::ArrayHandle<vtkm::Id> >
                     permuteIndexOffsets(cellIds, 
                                         cellSet.GetIndexOffsetArray(vtkm::TopologyElementTagPoint(),
                                                                     vtkm::TopologyElementTagCell()));

    // Output cell set components
    vtkm::cont::ArrayHandle<vtkm::UInt8> outShapes;
    DeviceAlgorithm::Copy(permuteShapes, outShapes);

    vtkm::cont::ArrayHandle<vtkm::IdComponent> outNumIndices;
    DeviceAlgorithm::Copy(permuteNumIndices, outNumIndices);

    vtkm::cont::ArrayHandle<vtkm::IdComponent> inOffset;
    DeviceAlgorithm::Copy(permuteIndexOffsets, inOffset);

    // Calculate the offset of the output indices
    vtkm::cont::ArrayHandle<vtkm::IdComponent> outIndexOffsets;
    DeviceAlgorithm::ScanExclusive(outNumIndices, outIndexOffsets);

    // Size of connectivity
    vtkm::Id sizeConnectivity = DeviceAlgorithm::Reduce(outNumIndices, 0);

    // Using the input connectivity, empty output connectivity, and index offset, and numIndices
    vtkm::cont::ArrayHandle<vtkm::Id> outConnectivity;
    outConnectivity.Allocate(sizeConnectivity);

    // Build new cells
    BuildConnectivity buildConnectivityWorklet;
    vtkm::worklet::DispatcherMapField<BuildConnectivity,DeviceAdapter>
        buildConnectivityDispatcher(buildConnectivityWorklet);
    buildConnectivityDispatcher.Invoke(outNumIndices,
                                       inOffset,
                                       outIndexOffsets,
                                       cellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                                                    vtkm::TopologyElementTagCell()),
                                       outConnectivity);

    // Build the output cell set
    vtkm::cont::CellSetExplicit< > outCellSet("cells");
    outCellSet.Fill(cellIds.GetNumberOfValues(),
                    outShapes,
                    outNumIndices,
                    outConnectivity);

    return outCellSet;
  }

  // Extract by cell ids
  template <typename DeviceAdapter>
  vtkm::cont::CellSetExplicit<> Run(const vtkm::cont::CellSetExplicit<> &cellSet,
                                    const vtkm::cont::ArrayHandle<vtkm::Id> &cellIds,
                                    DeviceAdapter device)
  {
    return DoExtract(cellSet, cellIds, device); 
  }

  // Extract by ImplicitFunction volume of interest
  template <typename ImplicitFunction,
            typename DeviceAdapter>
  vtkm::cont::CellSetExplicit<> Run(
                                    const vtkm::cont::CellSetExplicit<> &cellSet,
                                    const ImplicitFunction &implicitFunction,
                                    const vtkm::cont::CoordinateSystem &coordinates,
                                    DeviceAdapter device)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;

    // Mask array to mark if a cell is within the volume of interest
    vtkm::cont::ArrayHandle<vtkm::IdComponent> maskArray;
    vtkm::Id numberOfInputCells = cellSet.GetNumberOfCells();
    DeviceAlgorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(0, numberOfInputCells),
                          maskArray);

    // Worklet output will be a boolean passFlag array
    typedef ExtractCellsWithImplicitFunction<ImplicitFunction> ExtractCellsWorklet;
    ExtractCellsWorklet worklet(implicitFunction);
    DispatcherMapField<ExtractCellsWorklet, DeviceAdapter> dispatcher(worklet);
    dispatcher.Invoke(cellSet.GetNumIndicesArray(vtkm::TopologyElementTagPoint(),
                                                 vtkm::TopologyElementTagCell()),
                      cellSet.GetIndexOffsetArray(vtkm::TopologyElementTagPoint(),
                                                 vtkm::TopologyElementTagCell()),
                      cellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                                 vtkm::TopologyElementTagCell()),
                      coordinates,
                      maskArray);

    vtkm::worklet::ScatterCounting CellScatter(maskArray, DeviceAdapter(), true);
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds = CellScatter.GetOutputToInputMap();

    // With the cell ids call the ExtractCellById code
    return DoExtract(cellSet, cellIds, device);
  }
};

}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_ExtractCells_h
