//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_TetrahedralizeExplicit_h
#define vtk_m_worklet_TetrahedralizeExplicit_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DynamicArrayHandle.h>
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

/// \brief Compute the tetrahedralize cells for an explicit grid data set
template <typename DeviceAdapter>
class TetrahedralizeExplicit
{
public:
  TetrahedralizeExplicit() {}

  //
  // Worklet to count the number of tetrahedra generated per cell
  //
  class TetrahedraPerCell : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<> shapes, ExecObject tables, FieldOut<> tetrahedronCount);
    typedef _3 ExecutionSignature(_1, _2);
    typedef _1 InputDomain;

    VTKM_CONT
    TetrahedraPerCell() {}

    VTKM_EXEC
    vtkm::IdComponent operator()(
      vtkm::UInt8 shape,
      const vtkm::worklet::internal::TetrahedralizeTablesExecutionObject<DeviceAdapter>& tables)
      const
    {
      return tables.GetCount(vtkm::CellShapeTagGeneric(shape));
    }
  };

  //
  // Worklet to turn cells into tetrahedra
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TetrahedralizeCell : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  ExecObject tables,
                                  FieldOutCell<> connectivityOut);
    typedef void ExecutionSignature(CellShape, PointIndices, _2, _3, VisitIndex);
    typedef _1 InputDomain;

    typedef vtkm::worklet::ScatterCounting ScatterType;
    VTKM_CONT
    ScatterType GetScatter() const { return this->Scatter; }

    template <typename CellArrayType>
    VTKM_CONT TetrahedralizeCell(const CellArrayType& cellArray)
      : Scatter(cellArray, DeviceAdapter())
    {
    }

    // Each cell produces tetrahedra and write result at the offset
    template <typename CellShapeTag, typename ConnectivityInVec, typename ConnectivityOutVec>
    VTKM_EXEC void operator()(
      CellShapeTag shape,
      const ConnectivityInVec& connectivityIn,
      const vtkm::worklet::internal::TetrahedralizeTablesExecutionObject<DeviceAdapter>& tables,
      ConnectivityOutVec& connectivityOut,
      vtkm::IdComponent visitIndex) const
    {
      vtkm::Vec<vtkm::IdComponent, 4> tetIndices = tables.GetIndices(shape, visitIndex);
      connectivityOut[0] = connectivityIn[tetIndices[0]];
      connectivityOut[1] = connectivityIn[tetIndices[1]];
      connectivityOut[2] = connectivityIn[tetIndices[2]];
      connectivityOut[3] = connectivityIn[tetIndices[3]];
    }

  private:
    ScatterType Scatter;
  };

  template <typename CellSetType>
  vtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      vtkm::cont::ArrayHandle<vtkm::IdComponent>& outCellsPerCell)
  {
    vtkm::cont::CellSetSingleType<> outCellSet(cellSet.GetName());

    // Input topology
    auto inShapes =
      cellSet.GetShapesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
    auto inNumIndices =
      cellSet.GetNumIndicesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

    // Output topology
    vtkm::cont::ArrayHandle<vtkm::Id> outConnectivity;

    vtkm::worklet::internal::TetrahedralizeTables tables;

    // Determine the number of output cells each input cell will generate
    vtkm::worklet::DispatcherMapField<TetrahedraPerCell, DeviceAdapter> tetPerCellDispatcher;
    tetPerCellDispatcher.Invoke(inShapes, tables.PrepareForInput(DeviceAdapter()), outCellsPerCell);

    // Build new cells
    TetrahedralizeCell tetrahedralizeWorklet(outCellsPerCell);
    vtkm::worklet::DispatcherMapTopology<TetrahedralizeCell, DeviceAdapter>
      tetrahedralizeDispatcher(tetrahedralizeWorklet);
    tetrahedralizeDispatcher.Invoke(cellSet,
                                    tables.PrepareForInput(DeviceAdapter()),
                                    vtkm::cont::make_ArrayHandleGroupVec<4>(outConnectivity));

    // Add cells to output cellset
    outCellSet.Fill(cellSet.GetNumberOfPoints(), vtkm::CellShapeTagTetra::Id, 4, outConnectivity);
    return outCellSet;
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_TetrahedralizeExplicit_h
