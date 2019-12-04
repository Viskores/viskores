//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
  struct CountCellPoints : vtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn inputTopology, FieldOut numPointsInCell);
    using ExecutionSignature = _2(PointCount);

    VTKM_EXEC
    vtkm::IdComponent operator()(vtkm::IdComponent numPoints) const { return numPoints; }
  };

  struct PassCellStructure : vtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn inputTopology, FieldOut shapes, FieldOut pointIndices);
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
            typename ConnectivityStorage,
            typename OffsetsStorage>
  VTKM_CONT static void Run(
    const InCellSetType& inCellSet,
    vtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& outCellSet)
  {
    VTKM_IS_DYNAMIC_OR_STATIC_CELL_SET(InCellSetType);

    vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;

    vtkm::worklet::DispatcherMapTopology<CountCellPoints> countDispatcher;
    countDispatcher.Invoke(inCellSet, numIndices);

    vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorage> shapes;
    vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorage> connectivity;

    vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage> offsets;
    vtkm::Id connectivitySize;
    vtkm::cont::ConvertNumIndicesToOffsets(numIndices, offsets, connectivitySize);
    connectivity.Allocate(connectivitySize);

    auto offsetsTrim =
      vtkm::cont::make_ArrayHandleView(offsets, 0, offsets.GetNumberOfValues() - 1);

    vtkm::worklet::DispatcherMapTopology<PassCellStructure> passDispatcher;
    passDispatcher.Invoke(
      inCellSet, shapes, vtkm::cont::make_ArrayHandleGroupVecVariable(connectivity, offsetsTrim));

    vtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage> newCellSet;
    newCellSet.Fill(inCellSet.GetNumberOfPoints(), shapes, connectivity, offsets);
    outCellSet = newCellSet;
  }

  template <typename InCellSetType>
  VTKM_CONT static vtkm::cont::CellSetExplicit<> Run(const InCellSetType& inCellSet)
  {
    VTKM_IS_DYNAMIC_OR_STATIC_CELL_SET(InCellSetType);

    vtkm::cont::CellSetExplicit<> outCellSet;
    Run(inCellSet, outCellSet);

    return outCellSet;
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_CellDeepCopy_h
