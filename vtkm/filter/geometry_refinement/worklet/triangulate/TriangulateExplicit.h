//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_TriangulateExplicit_h
#define vtk_m_worklet_TriangulateExplicit_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/worklet/internal/TriangulateTables.h>

namespace vtkm
{
namespace worklet
{

/// \brief Compute the triangulate cells for an explicit grid data set
class TriangulateExplicit
{
public:
  TriangulateExplicit() {}

  //
  // Worklet to count the number of triangles generated per cell
  //
  class TrianglesPerCell : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cells, ExecObject tables, FieldOut triangleCount);
    using ExecutionSignature = _3(CellShape, IncidentElementCount, _2);
    using InputDomain = _1;

    VTKM_CONT
    TrianglesPerCell() {}

    template <typename CellShapeTag>
    VTKM_EXEC vtkm::IdComponent operator()(
      CellShapeTag shape,
      vtkm::IdComponent numPoints,
      const vtkm::worklet::internal::TriangulateTablesExecutionObject& tables) const
    {
      return tables.GetCount(shape, numPoints);
    }
  };

  //
  // Worklet to turn cells into triangles
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TriangulateCell : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  ExecObject tables,
                                  FieldOutCell connectivityOut);
    using ExecutionSignature = void(CellShape, PointIndices, _2, _3, VisitIndex);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template <typename CountArrayType>
    VTKM_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
    {
      return ScatterType(countArray);
    }

    // Each cell produces triangles and write result at the offset
    template <typename CellShapeTag, typename ConnectivityInVec, typename ConnectivityOutVec>
    VTKM_EXEC void operator()(
      CellShapeTag shape,
      const ConnectivityInVec& connectivityIn,
      const vtkm::worklet::internal::TriangulateTablesExecutionObject& tables,
      ConnectivityOutVec& connectivityOut,
      vtkm::IdComponent visitIndex) const
    {
      vtkm::IdComponent3 triIndices = tables.GetIndices(shape, visitIndex);
      connectivityOut[0] = connectivityIn[triIndices[0]];
      connectivityOut[1] = connectivityIn[triIndices[1]];
      connectivityOut[2] = connectivityIn[triIndices[2]];
    }
  };
  template <typename CellSetType>
  vtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      vtkm::cont::ArrayHandle<vtkm::IdComponent>& outCellsPerCell)
  {
    vtkm::cont::CellSetSingleType<> outCellSet;

    vtkm::cont::Invoker invoke;

    // Output topology
    vtkm::cont::ArrayHandle<vtkm::Id> outConnectivity;

    vtkm::worklet::internal::TriangulateTables tables;

    // Determine the number of output cells each input cell will generate
    invoke(TrianglesPerCell{}, cellSet, tables, outCellsPerCell);

    // Build new cells
    invoke(TriangulateCell{},
           TriangulateCell::MakeScatter(outCellsPerCell),
           cellSet,
           tables,
           vtkm::cont::make_ArrayHandleGroupVec<3>(outConnectivity));

    // Add cells to output cellset
    outCellSet.Fill(
      cellSet.GetNumberOfPoints(), vtkm::CellShapeTagTriangle::Id, 3, outConnectivity);
    return outCellSet;
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_TriangulateExplicit_h
