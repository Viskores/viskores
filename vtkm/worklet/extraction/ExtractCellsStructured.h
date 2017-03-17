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
#ifndef vtkm_m_worklet_ExtractCellsStructured_h
#define vtkm_m_worklet_ExtractCellsStructured_h

#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/ImplicitFunctions.h>
#include <vtkm/Types.h>

namespace vtkm {
namespace worklet {

class ExtractCellsStructured : public vtkm::worklet::WorkletMapPointToCell
{
public:
  ExtractCellsStructured() {}

  // Set mask for any cell whose points are inside volume of interest
  template<typename ImplicitFunction>
  class ExtractCellsWithImplicitFunction : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  WholeArrayIn<Vec3> coordinates,
                                  FieldOut<IdComponentType> mask);
    typedef   _3 ExecutionSignature(PointCount, PointIndices, _2);

    VTKM_CONT
    ExtractCellsWithImplicitFunction(const ImplicitFunction &function) : Function(function) {}

    template <typename ConnectivityInVec, typename InVecFieldPortalType>
    VTKM_CONT
    vtkm::IdComponent operator()(const vtkm::IdComponent &numIndices,
                                 const ConnectivityInVec &connectivityIn,
                                 const InVecFieldPortalType &coordinates) const
    {
      // If any point is outside volume of interest, cell is also
      vtkm::IdComponent mask = 1;
      for (vtkm::Id indx = 0; indx < numIndices; indx++)
      {
        vtkm::Id ptId = connectivityIn[indx];
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

  // Create mask array from selected cell ids for use in ScatterCounting
  class CreateMask : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType> cellId,
                                  WholeArrayOut<IdType> maskArray);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT
    CreateMask() {}

    template <typename OutPortalType>
    VTKM_EXEC
    void operator()(const vtkm::Id &cellId,
                          OutPortalType &maskArray) const
    {
      maskArray.Set(cellId, 1);
    }
  };


  // Build the connectivity of the output cellset using input and mask of extracted cells
  class BuildConnectivity : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  FieldOutCell<> connectivityOut);
    typedef void ExecutionSignature(InputIndex, PointCount, PointIndices, _2);
    typedef _1 InputDomain;

    typedef vtkm::worklet::ScatterCounting ScatterType;
    VTKM_CONT
    ScatterType GetScatter() const
    {
      return Scatter;
    }

    template <typename DeviceAdapter>
    VTKM_CONT
    BuildConnectivity(vtkm::cont::ArrayHandle<vtkm::IdComponent> maskArray, 
                      DeviceAdapter) : 
                                  Scatter(maskArray, DeviceAdapter()) {}

    template<typename ConnectivityInVec, typename ConnectivityOutVec>
    VTKM_EXEC
    void operator()(const vtkm::Id& workIndex,
                    const vtkm::IdComponent& pointCount,
                    const ConnectivityInVec &connectivityIn,
                    ConnectivityOutVec &connectivityOut) const
    {
      for (vtkm::IdComponent indx = 0; indx < pointCount; indx++)
      {
        connectivityOut[indx] = connectivityIn[indx];
std::cout << "workIndex " << workIndex << "  connectivity out " << indx << "  " << connectivityOut[indx] << std::endl;
      }
    }

  private:
    ScatterType Scatter;
  };

  // Extract cells by id and building new connectivity using original point ids
  template <typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> DoExtract2D(
                                            const vtkm::cont::CellSetStructured<2> &cellSet,
                                            const vtkm::cont::ArrayHandle<vtkm::Id> &cellIds,
                                            const vtkm::cont::ArrayHandle<vtkm::IdComponent> &maskArray,
                                            DeviceAdapter device)
  {
    (void) device;

    // Create connectivity using rules for point ids around cells
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

    // Build new cell connectivity using ScatterCounting on maskArray
    BuildConnectivity buildConnectivity(maskArray, DeviceAdapter());
    vtkm::worklet::DispatcherMapTopology<BuildConnectivity,DeviceAdapter> dispatcher(buildConnectivity);
    dispatcher.Invoke(cellSet,
                      vtkm::cont::make_ArrayHandleGroupVec<4>(connectivity));
std::cout << "Output connectivity size " << connectivity.GetNumberOfValues() << std::endl;

    // Build the output cell set
    vtkm::cont::CellSetSingleType< > outCellSet("cells");
    outCellSet.Fill(cellIds.GetNumberOfValues(),
                    vtkm::CellShapeTagQuad::Id,
                    4,
                    connectivity);

    return outCellSet;
  }

  // Extract cells by id and building new connectivity using original point ids
  template <typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> DoExtract3D(
                                            const vtkm::cont::CellSetStructured<3> &cellSet,
                                            const vtkm::cont::ArrayHandle<vtkm::Id> &cellIds,
                                            const vtkm::cont::ArrayHandle<vtkm::IdComponent> &maskArray,
                                            DeviceAdapter device)
  {
    (void) device;

    // Create connectivity using rules for point ids around cells
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

    // Build new cell connectivity using ScatterCounting on maskArray
    BuildConnectivity buildConnectivity(maskArray, DeviceAdapter());
    vtkm::worklet::DispatcherMapTopology<BuildConnectivity,DeviceAdapter> dispatcher(buildConnectivity);
    dispatcher.Invoke(cellSet,
                      vtkm::cont::make_ArrayHandleGroupVec<8>(connectivity));
std::cout << "Output connectivity size " << connectivity.GetNumberOfValues() << std::endl;

    // Build the output cell set
    vtkm::cont::CellSetSingleType< > outCellSet("cells");
    outCellSet.Fill(cellIds.GetNumberOfValues(),
                    vtkm::CellShapeTagHexahedron::Id,
                    8,
                    connectivity);

    return outCellSet;
  }


  // Extract by cell ids
  template <typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetStructured<2> &cellSet,
                                      const vtkm::cont::ArrayHandle<vtkm::Id> &cellIds,
                                      DeviceAdapter device)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;

    // Create connectivity using rules for point ids around cells
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

    // Turn the input cell id array into a mask for use in ScatterCounting
    vtkm::cont::ArrayHandle<vtkm::IdComponent> maskArray;
    vtkm::Id numberOfInputCells = cellSet.GetNumberOfCells();
    DeviceAlgorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(0, numberOfInputCells),
                          maskArray);

    // Worklet to turn on the cell ids to be extracted
    CreateMask createMask;
    vtkm::worklet::DispatcherMapField<CreateMask> dispatcherCreateMask;
    dispatcherCreateMask.Invoke(cellIds,
                                maskArray);

    return DoExtract2D(cellSet, cellIds, maskArray, device);
  }

  // Extract by cell ids
  template <typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetStructured<3> &cellSet,
                                      const vtkm::cont::ArrayHandle<vtkm::Id> &cellIds,
                                      DeviceAdapter device)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;

    // Create connectivity using rules for point ids around cells
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

    // Turn the input cell id array into a mask for use in ScatterCounting
    vtkm::cont::ArrayHandle<vtkm::IdComponent> maskArray;
    vtkm::Id numberOfInputCells = cellSet.GetNumberOfCells();
    DeviceAlgorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(0, numberOfInputCells),
                          maskArray);

    // Worklet to turn on the cell ids to be extracted
    CreateMask createMask;
    vtkm::worklet::DispatcherMapField<CreateMask> dispatcherCreateMask;
    dispatcherCreateMask.Invoke(cellIds,
                                maskArray);

    return DoExtract3D(cellSet, cellIds, maskArray, device);
  }

  // Extract by ImplicitFunction volume of interest
  template <typename ImplicitFunction,
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetStructured<2> &cellSet,
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
    DispatcherMapTopology<ExtractCellsWorklet, DeviceAdapter> dispatcher(worklet);
    dispatcher.Invoke(cellSet,
                      coordinates,
                      maskArray);

    vtkm::worklet::ScatterCounting CellScatter(maskArray, DeviceAdapter(), true);
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds = CellScatter.GetOutputToInputMap();

    return DoExtract2D(cellSet, cellIds, maskArray, device);
  }

  // Extract by ImplicitFunction volume of interest
  template <typename ImplicitFunction,
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetStructured<3> &cellSet,
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
    DispatcherMapTopology<ExtractCellsWorklet, DeviceAdapter> dispatcher(worklet);
    dispatcher.Invoke(cellSet,
                      coordinates,
                      maskArray);

    vtkm::worklet::ScatterCounting CellScatter(maskArray, DeviceAdapter(), true);
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds = CellScatter.GetOutputToInputMap();

    return DoExtract3D(cellSet, cellIds, maskArray, device);
  }
};

}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_ExtractCellsStructured_h
