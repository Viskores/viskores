//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_CellDeepCopy_h
#define vtk_m_worklet_CellDeepCopy_h

#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace worklet
{

/// Container for worklets and helper methods to copy a cell set to a new
/// \c CellSetExplicit structure
///
struct CellDeepCopy
{
  struct CountCellPoints : vtkm::worklet::WorkletMapPointToCell
  {
    using ControlSignature = void(CellSetIn inputTopology, FieldOut<> numPointsInCell);
    using ExecutionSignature = _2(PointCount);

    VTKM_EXEC
    vtkm::IdComponent operator()(vtkm::IdComponent numPoints) const { return numPoints; }
  };

  struct PassCellStructure : vtkm::worklet::WorkletMapPointToCell
  {
    using ControlSignature = void(CellSetIn inputTopology,
                                  FieldOut<> shapes,
                                  FieldOut<> pointIndices);
    using ExecutionSignature = void(CellShape, PointIndices, _2, _3);

    template <typename CellShape, typename InPointIndexType, typename OutPointIndexType>
    VTKM_EXEC void operator()(const CellShape& inShape,
                              const InPointIndexType& inPoints,
                              vtkm::UInt8& outShape,
                              OutPointIndexType& outPoints) const
    {
      (void)inShape; //C4100 false positive workaround
      outShape = inShape.Id;

      vtkm::IdComponent numPoints = inPoints.GetNumberOfComponents();
      VTKM_ASSERT(numPoints == outPoints.GetNumberOfComponents());
      for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
      {
        outPoints[pointIndex] = inPoints[pointIndex];
      }
    }
  };

  template <typename InCellSetType,
            typename ShapeStorage,
            typename NumIndicesStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage,
            typename Device>
  VTKM_CONT static void Run(const InCellSetType& inCellSet,
                            vtkm::cont::CellSetExplicit<ShapeStorage,
                                                        NumIndicesStorage,
                                                        ConnectivityStorage,
                                                        OffsetsStorage>& outCellSet,
                            Device)
  {
    VTKM_IS_DYNAMIC_OR_STATIC_CELL_SET(InCellSetType);

    vtkm::cont::ArrayHandle<vtkm::IdComponent, NumIndicesStorage> numIndices;

    vtkm::worklet::DispatcherMapTopology<CountCellPoints, Device> countDispatcher;
    countDispatcher.Invoke(inCellSet, numIndices);

    vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorage> shapes;
    vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorage> connectivity;

    vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage> offsets;
    vtkm::Id connectivitySize;
    vtkm::cont::ConvertNumComponentsToOffsets(numIndices, offsets, connectivitySize);
    connectivity.Allocate(connectivitySize);

    vtkm::worklet::DispatcherMapTopology<PassCellStructure, Device> passDispatcher;
    passDispatcher.Invoke(
      inCellSet, shapes, vtkm::cont::make_ArrayHandleGroupVecVariable(connectivity, offsets));

    vtkm::cont::
      CellSetExplicit<ShapeStorage, NumIndicesStorage, ConnectivityStorage, OffsetsStorage>
        newCellSet(inCellSet.GetName());
    newCellSet.Fill(inCellSet.GetNumberOfPoints(), shapes, numIndices, connectivity, offsets);
    outCellSet = newCellSet;
  }

  template <typename InCellSetType, typename Device>
  VTKM_CONT static vtkm::cont::CellSetExplicit<> Run(const InCellSetType& inCellSet, Device)
  {
    VTKM_IS_DYNAMIC_OR_STATIC_CELL_SET(InCellSetType);

    vtkm::cont::CellSetExplicit<> outCellSet;
    Run(inCellSet, outCellSet, Device());

    return outCellSet;
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_CellDeepCopy_h
