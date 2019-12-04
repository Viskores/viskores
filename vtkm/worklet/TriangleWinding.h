//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtkm_m_worklet_TriangleWinding_h
#define vtkm_m_worklet_TriangleWinding_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/ArrayRangeCompute.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/MaskIndices.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>

namespace vtkm
{
namespace worklet
{

/**
 * This worklet ensures that triangle windings are consistent with provided
 * cell normals. The triangles are wound CCW around the cell normals, and
 * all other cells are ignored.
 *
 * The input cellset must be unstructured.
 */
class TriangleWinding
{
public:
  // Used by Explicit and SingleType specializations
  struct WorkletWindToCellNormals : public WorkletMapField
  {
    using ControlSignature = void(FieldIn cellNormals, FieldInOut cellPoints, WholeArrayIn coords);
    using ExecutionSignature = void(_1 cellNormal, _2 cellPoints, _3 coords);

    template <typename NormalCompType, typename CellPointsType, typename CoordsPortal>
    VTKM_EXEC void operator()(const vtkm::Vec<NormalCompType, 3>& cellNormal,
                              CellPointsType& cellPoints,
                              const CoordsPortal& coords) const
    {
      // We only care about triangles:
      if (cellPoints.GetNumberOfComponents() != 3)
      {
        return;
      }

      using NormalType = vtkm::Vec<NormalCompType, 3>;

      const NormalType p0 = coords.Get(cellPoints[0]);
      const NormalType p1 = coords.Get(cellPoints[1]);
      const NormalType p2 = coords.Get(cellPoints[2]);
      const NormalType v01 = p1 - p0;
      const NormalType v02 = p2 - p0;
      const NormalType triangleNormal = vtkm::Cross(v01, v02);
      if (vtkm::Dot(cellNormal, triangleNormal) < 0)
      {
        // Can't just use std::swap from exec function:
        const vtkm::Id tmp = cellPoints[1];
        cellPoints[1] = cellPoints[2];
        cellPoints[2] = tmp;
      }
    }
  };

  // Used by generic implementations:
  struct WorkletGetCellShapesAndSizes : public WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn cells, FieldOutCell shapes, FieldOutCell sizes);
    using ExecutionSignature = void(CellShape, PointCount, _2, _3);

    template <typename CellShapeTag>
    VTKM_EXEC void operator()(const CellShapeTag cellShapeIn,
                              const vtkm::IdComponent cellSizeIn,
                              vtkm::UInt8& cellShapeOut,
                              vtkm::IdComponent& cellSizeOut) const
    {
      cellSizeOut = cellSizeIn;
      cellShapeOut = cellShapeIn.Id;
    }
  };

  struct WorkletWindToCellNormalsGeneric : public WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn cellsIn,
                                  WholeArrayIn coords,
                                  FieldInCell cellNormals,
                                  FieldOutCell cellsOut);
    using ExecutionSignature = void(PointIndices, _2, _3, _4);

    template <typename InputIds, typename Coords, typename Normal, typename OutputIds>
    VTKM_EXEC void operator()(const InputIds& inputIds,
                              const Coords& coords,
                              const Normal& normal,
                              OutputIds& outputIds) const
    {
      VTKM_ASSERT(inputIds.GetNumberOfComponents() == outputIds.GetNumberOfComponents());

      // We only care about triangles:
      if (inputIds.GetNumberOfComponents() != 3)
      {
        // Just passthrough non-triangles
        // Cannot just assign here, must do a manual component-wise copy to
        // support VecFromPortal:
        for (vtkm::IdComponent i = 0; i < inputIds.GetNumberOfComponents(); ++i)
        {
          outputIds[i] = inputIds[i];
        }
        return;
      }

      const Normal p0 = coords.Get(inputIds[0]);
      const Normal p1 = coords.Get(inputIds[1]);
      const Normal p2 = coords.Get(inputIds[2]);
      const Normal v01 = p1 - p0;
      const Normal v02 = p2 - p0;
      const Normal triangleNormal = vtkm::Cross(v01, v02);
      if (vtkm::Dot(normal, triangleNormal) < 0)
      { // Reorder triangle:
        outputIds[0] = inputIds[0];
        outputIds[1] = inputIds[2];
        outputIds[2] = inputIds[1];
      }
      else
      { // passthrough:
        outputIds[0] = inputIds[0];
        outputIds[1] = inputIds[1];
        outputIds[2] = inputIds[2];
      }
    }
  };

  struct Launcher
  {
    vtkm::cont::DynamicCellSet Result;

    // Generic handler:
    template <typename CellSetType,
              typename PointComponentType,
              typename PointStorageType,
              typename CellNormalComponentType,
              typename CellNormalStorageType>
    VTKM_CONT void operator()(
      const CellSetType& cellSet,
      const vtkm::cont::ArrayHandle<vtkm::Vec<PointComponentType, 3>, PointStorageType>& coords,
      const vtkm::cont::ArrayHandle<vtkm::Vec<CellNormalComponentType, 3>, CellNormalStorageType>&
        cellNormals,
      ...)
    {
      const auto numCells = cellSet.GetNumberOfCells();
      if (numCells == 0)
      {
        this->Result = cellSet;
        return;
      }

      vtkm::cont::Invoker invoker;

      // Get each cell's size:
      vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
      vtkm::cont::ArrayHandle<vtkm::UInt8> cellShapes;
      {
        WorkletGetCellShapesAndSizes worklet;
        invoker(worklet, cellSet, cellShapes, numIndices);
      }

      // Check to see if we can use CellSetSingleType:
      vtkm::IdComponent cellSize = 0; // 0 if heterogeneous, >0 if homogeneous
      vtkm::UInt8 cellShape = 0;      // only valid if homogeneous
      {
        auto rangeHandleSizes = vtkm::cont::ArrayRangeCompute(numIndices);
        auto rangeHandleShapes = vtkm::cont::ArrayRangeCompute(cellShapes);

        cellShapes.ReleaseResourcesExecution();

        auto rangeSizes = rangeHandleSizes.GetPortalConstControl().Get(0);
        auto rangeShapes = rangeHandleShapes.GetPortalConstControl().Get(0);

        const bool sameSize = vtkm::Abs(rangeSizes.Max - rangeSizes.Min) < 0.5;
        const bool sameShape = vtkm::Abs(rangeShapes.Max - rangeShapes.Min) < 0.5;

        if (sameSize && sameShape)
        {
          cellSize = static_cast<vtkm::IdComponent>(rangeSizes.Min + 0.5);
          cellShape = static_cast<vtkm::UInt8>(rangeShapes.Min + 0.5);
        }
      }

      if (cellSize > 0)
      { // Single cell type:
        // don't need these anymore:
        numIndices.ReleaseResources();
        cellShapes.ReleaseResources();

        vtkm::cont::ArrayHandle<vtkm::Id> conn;
        conn.Allocate(cellSize * numCells);

        auto offsets = vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, cellSize, numCells);
        auto connGroupVec = vtkm::cont::make_ArrayHandleGroupVecVariable(conn, offsets);

        WorkletWindToCellNormalsGeneric worklet;
        invoker(worklet, cellSet, coords, cellNormals, connGroupVec);

        vtkm::cont::CellSetSingleType<> outCells;
        outCells.Fill(cellSet.GetNumberOfPoints(), cellShape, cellSize, conn);
        this->Result = outCells;
      }
      else
      { // Multiple cell types:
        vtkm::cont::ArrayHandle<vtkm::Id> offsets;
        vtkm::Id connSize;
        vtkm::cont::ConvertNumIndicesToOffsets(numIndices, offsets, connSize);
        numIndices.ReleaseResourcesExecution();

        vtkm::cont::ArrayHandle<vtkm::Id> conn;
        conn.Allocate(connSize);

        // Trim the last value off for the group vec array:
        auto offsetsTrim =
          vtkm::cont::make_ArrayHandleView(offsets, 0, offsets.GetNumberOfValues() - 1);
        auto connGroupVec = vtkm::cont::make_ArrayHandleGroupVecVariable(conn, offsetsTrim);

        WorkletWindToCellNormalsGeneric worklet;
        invoker(worklet, cellSet, coords, cellNormals, connGroupVec);

        vtkm::cont::CellSetExplicit<> outCells;
        outCells.Fill(cellSet.GetNumberOfPoints(), cellShapes, conn, offsets);
        this->Result = outCells;
      }
    }

    // Specialization for CellSetExplicit
    template <typename S,
              typename C,
              typename O,
              typename PointComponentType,
              typename PointStorageType,
              typename CellNormalComponentType,
              typename CellNormalStorageType>
    VTKM_CONT void operator()(
      const vtkm::cont::CellSetExplicit<S, C, O>& cellSet,
      const vtkm::cont::ArrayHandle<vtkm::Vec<PointComponentType, 3>, PointStorageType>& coords,
      const vtkm::cont::ArrayHandle<vtkm::Vec<CellNormalComponentType, 3>, CellNormalStorageType>&
        cellNormals,
      int)
    {
      using WindToCellNormals = vtkm::worklet::DispatcherMapField<WorkletWindToCellNormals>;

      const auto numCells = cellSet.GetNumberOfCells();
      if (numCells == 0)
      {
        this->Result = cellSet;
        return;
      }

      vtkm::cont::ArrayHandle<vtkm::Id> conn;
      {
        const auto& connIn = cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell{},
                                                          vtkm::TopologyElementTagPoint{});
        vtkm::cont::Algorithm::Copy(connIn, conn);
      }

      const auto& offsets =
        cellSet.GetOffsetsArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
      auto offsetsTrim =
        vtkm::cont::make_ArrayHandleView(offsets, 0, offsets.GetNumberOfValues() - 1);
      auto cells = vtkm::cont::make_ArrayHandleGroupVecVariable(conn, offsetsTrim);

      WindToCellNormals dispatcher;
      dispatcher.Invoke(cellNormals, cells, coords);

      const auto& shapes =
        cellSet.GetShapesArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
      vtkm::cont::CellSetExplicit<S, vtkm::cont::StorageTagBasic, O> newCells;
      newCells.Fill(cellSet.GetNumberOfPoints(), shapes, conn, offsets);

      this->Result = newCells;
    }

    // Specialization for CellSetSingleType
    template <typename C,
              typename PointComponentType,
              typename PointStorageType,
              typename CellNormalComponentType,
              typename CellNormalStorageType>
    VTKM_CONT void operator()(
      const vtkm::cont::CellSetSingleType<C>& cellSet,
      const vtkm::cont::ArrayHandle<vtkm::Vec<PointComponentType, 3>, PointStorageType>& coords,
      const vtkm::cont::ArrayHandle<vtkm::Vec<CellNormalComponentType, 3>, CellNormalStorageType>&
        cellNormals,
      int)
    {
      using WindToCellNormals = vtkm::worklet::DispatcherMapField<WorkletWindToCellNormals>;

      const auto numCells = cellSet.GetNumberOfCells();
      if (numCells == 0)
      {
        this->Result = cellSet;
        return;
      }

      vtkm::cont::ArrayHandle<vtkm::Id> conn;
      {
        const auto& connIn = cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell{},
                                                          vtkm::TopologyElementTagPoint{});
        vtkm::cont::Algorithm::Copy(connIn, conn);
      }

      const auto& offsets =
        cellSet.GetOffsetsArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
      auto offsetsTrim =
        vtkm::cont::make_ArrayHandleView(offsets, 0, offsets.GetNumberOfValues() - 1);
      auto cells = vtkm::cont::make_ArrayHandleGroupVecVariable(conn, offsetsTrim);

      WindToCellNormals dispatcher;
      dispatcher.Invoke(cellNormals, cells, coords);

      vtkm::cont::CellSetSingleType<vtkm::cont::StorageTagBasic> newCells;
      newCells.Fill(cellSet.GetNumberOfPoints(),
                    cellSet.GetCellShape(0),
                    cellSet.GetNumberOfPointsInCell(0),
                    conn);

      this->Result = newCells;
    }
  };

  template <typename CellSetType,
            typename PointComponentType,
            typename PointStorageType,
            typename CellNormalComponentType,
            typename CellNormalStorageType>
  VTKM_CONT static vtkm::cont::DynamicCellSet Run(
    const CellSetType& cellSet,
    const vtkm::cont::ArrayHandle<vtkm::Vec<PointComponentType, 3>, PointStorageType>& coords,
    const vtkm::cont::ArrayHandle<vtkm::Vec<CellNormalComponentType, 3>, CellNormalStorageType>&
      cellNormals)
  {
    Launcher launcher;
    // The last arg is just to help with overload resolution on the templated
    // Launcher::operator() method, so that the more specialized impls are
    // preferred over the generic one.
    vtkm::cont::CastAndCall(cellSet, launcher, coords, cellNormals, 0);
    return launcher.Result;
  }
};
}
} // end namespace vtkm::worklet

#endif // vtkm_m_worklet_TriangleWinding_h
