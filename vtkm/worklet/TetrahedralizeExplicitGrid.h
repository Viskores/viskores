//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_TetrahedralizeExplicitGrid_h
#define vtk_m_worklet_TetrahedralizeExplicitGrid_h

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

namespace vtkm {
namespace worklet {

/// \brief Compute the tetrahedralize cells for an explicit grid data set
template <typename DeviceAdapter>
class TetrahedralizeFilterExplicitGrid
{
public:

  //
  // Worklet to count the number of triangles generated per cell
  //
  class TrianglesPerCell : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<> shapes,
                                  FieldIn<> numPoints,
                                  ExecObject tables,
                                  FieldOut<> triangleCount);
    typedef _4 ExecutionSignature(_1,_2,_3);
    typedef _1 InputDomain;

    VTKM_CONT_EXPORT
    TrianglesPerCell() {}

    VTKM_EXEC_EXPORT
    vtkm::IdComponent operator()(
        vtkm::UInt8 shape,
        vtkm::IdComponent numPoints,
        const vtkm::worklet::internal::TriangulateTablesExecutionObject<DeviceAdapter> &tables) const
    {
      return tables.GetCount(vtkm::CellShapeTagGeneric(shape), numPoints);
    }
  };

  //
  // Worklet to count the number of tetrahedra generated per cell
  //
  class TetrahedraPerCell : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<> shapes,
                                  ExecObject tables,
                                  FieldOut<> triangleCount);
    typedef _3 ExecutionSignature(_1, _2);
    typedef _1 InputDomain;

    VTKM_CONT_EXPORT
    TetrahedraPerCell() {}

    VTKM_EXEC_EXPORT
    vtkm::IdComponent operator()(
        vtkm::UInt8 shape,
        const vtkm::worklet::internal::TetrahedralizeTablesExecutionObject<DeviceAdapter> &tables) const
    {
      return tables.GetCount(vtkm::CellShapeTagGeneric(shape));
    }
  };

  //
  // Worklet to turn cells into triangles
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TriangulateCell : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(TopologyIn topology,
                                  ExecObject tables,
                                  FieldOutCell<> connectivityOut);
    typedef void ExecutionSignature(CellShape, PointIndices, _2, _3, VisitIndex);
    typedef _1 InputDomain;

    typedef vtkm::worklet::ScatterCounting ScatterType;
    VTKM_CONT_EXPORT
    ScatterType GetScatter() const
    {
      return this->Scatter;
    }

    template<typename CountArrayType>
    VTKM_CONT_EXPORT
    TriangulateCell(const CountArrayType &countArray)
      : Scatter(countArray, DeviceAdapter())
    {  }

    // Each cell produces triangles and write result at the offset
    template<typename CellShapeTag,
             typename ConnectivityInVec,
             typename ConnectivityOutVec>
    VTKM_EXEC_EXPORT
    void operator()(
        CellShapeTag shape,
        const ConnectivityInVec &connectivityIn,
        const vtkm::worklet::internal::TriangulateTablesExecutionObject<DeviceAdapter> &tables,
        ConnectivityOutVec &connectivityOut,
        vtkm::IdComponent visitIndex) const
    {
      vtkm::Vec<vtkm::IdComponent,3> triIndices =
          tables.GetIndices(shape, visitIndex);
      connectivityOut[0] = connectivityIn[triIndices[0]];
      connectivityOut[1] = connectivityIn[triIndices[1]];
      connectivityOut[2] = connectivityIn[triIndices[2]];
    }

  private:
    ScatterType Scatter;
  };

  //
  // Worklet to turn cells into tetrahedra
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TetrahedralizeCell : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(TopologyIn topology,
                                  ExecObject tables,
                                  FieldOutCell<> connectivityOut);
    typedef void ExecutionSignature(CellShape, PointIndices, _2, _3, VisitIndex);
    typedef _1 InputDomain;

    typedef vtkm::worklet::ScatterCounting ScatterType;
    VTKM_CONT_EXPORT
    ScatterType GetScatter() const
    {
      return this->Scatter;
    }

    template<typename CellArrayType>
    VTKM_CONT_EXPORT
    TetrahedralizeCell(const CellArrayType &cellArray)
      : Scatter(cellArray, DeviceAdapter())
    {  }

    // Each cell produces tetrahedra and write result at the offset
    template<typename CellShapeTag,
             typename ConnectivityInVec,
             typename ConnectivityOutVec>
    VTKM_EXEC_EXPORT
    void operator()(CellShapeTag shape,
                    const ConnectivityInVec &connectivityIn,
                    const vtkm::worklet::internal::TetrahedralizeTablesExecutionObject<DeviceAdapter> &tables,
                    ConnectivityOutVec &connectivityOut,
                    vtkm::IdComponent visitIndex) const
    {
      vtkm::Vec<vtkm::IdComponent,4> tetIndices =
          tables.GetIndices(shape, visitIndex);
      connectivityOut[0] = connectivityIn[tetIndices[0]];
      connectivityOut[1] = connectivityIn[tetIndices[1]];
      connectivityOut[2] = connectivityIn[tetIndices[2]];
      connectivityOut[3] = connectivityIn[tetIndices[3]];
    }

  private:
    ScatterType Scatter;
  };

  //
  // Construct the filter to tetrahedralize explicit grid
  //
  TetrahedralizeFilterExplicitGrid(const vtkm::cont::DataSet &inDataSet,
                                         vtkm::cont::DataSet &outDataSet) :
    InDataSet(inDataSet),
    OutDataSet(outDataSet)
  {}

  vtkm::cont::DataSet InDataSet;  // input dataset with structured cell set
  vtkm::cont::DataSet OutDataSet; // output dataset with explicit cell set

  //
  // Populate the output dataset with triangles or tetrahedra based on input explicit dataset
  //
  void Run()
  {
    // Cell sets belonging to input and output datasets
    vtkm::cont::CellSetExplicit<> inCellSet;
    InDataSet.GetCellSet(0).CopyTo(inCellSet);
    vtkm::cont::CellSetSingleType<> &cellSet =
        this->OutDataSet.GetCellSet(0).template Cast<vtkm::cont::CellSetSingleType<> >();

    // Input dataset vertices and cell counts
    vtkm::Id dimensionality = inCellSet.GetDimensionality();

    // Input topology
    vtkm::cont::ArrayHandle<vtkm::UInt8> inShapes = inCellSet.GetShapesArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
    vtkm::cont::ArrayHandle<vtkm::IdComponent> inNumIndices = inCellSet.GetNumIndicesArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

    // Output topology
    vtkm::cont::ArrayHandle<vtkm::Id> outConnectivity;

    if (dimensionality == 2)
    {
      vtkm::worklet::internal::TriangulateTables tables;

      // Determine the number of output cells each input cell will generate
      vtkm::cont::ArrayHandle<vtkm::IdComponent> numOutCellArray;
      vtkm::worklet::DispatcherMapField<TrianglesPerCell,DeviceAdapter>
          triPerCellDispatcher;
      triPerCellDispatcher.Invoke(inShapes,
                                  inNumIndices,
                                  tables.PrepareForInput(DeviceAdapter()),
                                  numOutCellArray);

      // Build new cells
      TriangulateCell triangulateWorklet(numOutCellArray);
      vtkm::worklet::DispatcherMapTopology<TriangulateCell,DeviceAdapter>
          triangulateDispatcher(triangulateWorklet);
      triangulateDispatcher.Invoke(
            inCellSet,
            tables.PrepareForInput(DeviceAdapter()),
            vtkm::cont::make_ArrayHandleGroupVec<3>(outConnectivity));
    }
    else if (dimensionality == 3)
    {
      vtkm::worklet::internal::TetrahedralizeTables tables;

      // Determine the number of output cells each input cell will generate
      vtkm::cont::ArrayHandle<vtkm::IdComponent> numOutCellArray;
      vtkm::worklet::DispatcherMapField<TetrahedraPerCell,DeviceAdapter>
          tetPerCellDispatcher;
      tetPerCellDispatcher.Invoke(inShapes,
                                  tables.PrepareForInput(DeviceAdapter()),
                                  numOutCellArray);

      // Build new cells
      TetrahedralizeCell tetrahedralizeWorklet(numOutCellArray);
      vtkm::worklet::DispatcherMapTopology<TetrahedralizeCell,DeviceAdapter>
          tetrahedralizeDispatcher(tetrahedralizeWorklet);
      tetrahedralizeDispatcher.Invoke(
            inCellSet,
            tables.PrepareForInput(DeviceAdapter()),
            vtkm::cont::make_ArrayHandleGroupVec<4>(outConnectivity));
    }
    else
    {
      throw vtkm::cont::ErrorControlBadValue(
            "Unsupported dimensionality for TetrahedralizeExplicitGrid.");
    }

    // Add cells to output cellset
    cellSet.Fill(outConnectivity);
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_TetrahedralizeExplicitGrid_h
