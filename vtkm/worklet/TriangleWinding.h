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
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/DynamicCellSet.h>

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

  struct Launcher
  {
    vtkm::cont::DynamicCellSet Result;

    template <typename S,
              typename N,
              typename C,
              typename O,
              typename PointComponentType,
              typename PointStorageType,
              typename CellNormalComponentType,
              typename CellNormalStorageType>
    VTKM_CONT void operator()(
      const vtkm::cont::CellSetExplicit<S, N, C, O>& cellSet,
      const vtkm::cont::ArrayHandle<vtkm::Vec<PointComponentType, 3>, PointStorageType>& coords,
      const vtkm::cont::ArrayHandle<vtkm::Vec<CellNormalComponentType, 3>, CellNormalStorageType>&
        cellNormals)
    {
      using WindToCellNormals = vtkm::worklet::DispatcherMapField<WorkletWindToCellNormals>;

      vtkm::cont::ArrayHandle<vtkm::Id> conn;
      {
        const auto& connIn = cellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint{},
                                                          vtkm::TopologyElementTagCell{});
        vtkm::cont::Algorithm::Copy(connIn, conn);
      }

      const auto& offsets = cellSet.GetIndexOffsetArray(vtkm::TopologyElementTagPoint{},
                                                        vtkm::TopologyElementTagCell{});
      auto cells = vtkm::cont::make_ArrayHandleGroupVecVariable(conn, offsets);

      WindToCellNormals dispatcher;
      dispatcher.Invoke(cellNormals, cells, coords);

      const auto& shapes =
        cellSet.GetShapesArray(vtkm::TopologyElementTagPoint{}, vtkm::TopologyElementTagCell{});
      const auto& numIndices =
        cellSet.GetNumIndicesArray(vtkm::TopologyElementTagPoint{}, vtkm::TopologyElementTagCell{});
      vtkm::cont::CellSetExplicit<S, N, vtkm::cont::StorageTagBasic, O> newCells;
      newCells.Fill(cellSet.GetNumberOfPoints(), shapes, numIndices, conn, offsets);

      this->Result = newCells;
    }

    template <typename C,
              typename PointComponentType,
              typename PointStorageType,
              typename CellNormalComponentType,
              typename CellNormalStorageType>
    VTKM_CONT void operator()(
      const vtkm::cont::CellSetSingleType<C>& cellSet,
      const vtkm::cont::ArrayHandle<vtkm::Vec<PointComponentType, 3>, PointStorageType>& coords,
      const vtkm::cont::ArrayHandle<vtkm::Vec<CellNormalComponentType, 3>, CellNormalStorageType>&
        cellNormals)
    {
      using WindToCellNormals = vtkm::worklet::DispatcherMapField<WorkletWindToCellNormals>;

      vtkm::cont::ArrayHandle<vtkm::Id> conn;
      {
        const auto& connIn = cellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint{},
                                                          vtkm::TopologyElementTagCell{});
        vtkm::cont::Algorithm::Copy(connIn, conn);
      }

      const auto& offsets = cellSet.GetIndexOffsetArray(vtkm::TopologyElementTagPoint{},
                                                        vtkm::TopologyElementTagCell{});
      auto cells = vtkm::cont::make_ArrayHandleGroupVecVariable(conn, offsets);

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
    vtkm::cont::CastAndCall(cellSet, launcher, coords, cellNormals);
    return launcher.Result;
  }
};
}
} // end namespace vtkm::worklet

#endif // vtkm_m_worklet_TriangleWinding_h
