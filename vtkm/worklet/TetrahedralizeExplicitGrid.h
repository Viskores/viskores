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

#ifndef vtk_m_worklet_TetrahedralizeExplicitGrid_h
#define vtk_m_worklet_TetrahedralizeExplicitGrid_h

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/exec/ExecutionWholeArray.h>

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
                                  FieldIn<> numIndices,
                                  FieldOut<> triangleCount);
    typedef void ExecutionSignature(_1,_2,_3);
    typedef _1 InputDomain;

    VTKM_CONT_EXPORT
    TrianglesPerCell() {}
  
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &shape, 
                    const vtkm::Id &numIndices,
                    vtkm::Id &triangleCount) const
    {
      if (shape == vtkm::CELL_SHAPE_TRIANGLE)
        triangleCount = 1;
      else if (shape == vtkm::CELL_SHAPE_QUAD)
        triangleCount = 2;
      else if (shape == vtkm::CELL_SHAPE_POLYGON)
        triangleCount = numIndices - 2;
      else triangleCount = 0;
    }
  };

  //
  // Worklet to turn cells into triangles
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TriangulateCell : public vtkm::worklet::WorkletMapTopologyPointToCell
  {
  public:
    typedef void ControlSignature(FieldInTo<> triangleOffset,
                                  FieldInTo<> numIndices,
                                  TopologyIn topology,
                                  ExecObject connectivity);
    typedef void ExecutionSignature(_1,_2,_4, CellShape, FromIndices);
    typedef _3 InputDomain;

    VTKM_CONT_EXPORT
    TriangulateCell() {}

    // Each cell produces triangles and write result at the offset
    template<typename CellShapeTag, typename CellNodeVecType>
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &offset,
                    const vtkm::Id &numIndices,
                    vtkm::exec::ExecutionWholeArray<vtkm::Id> &connectivity,
                    CellShapeTag shape,
                    const CellNodeVecType &cellNodeIds) const
    {
      // Offset is in triangles, 3 vertices per triangle needed
      vtkm::Id startIndex = offset * 3;
      if (shape.Id == vtkm::CELL_SHAPE_TRIANGLE) {
        connectivity.Set(startIndex++, cellNodeIds[0]);
        connectivity.Set(startIndex++, cellNodeIds[1]);
        connectivity.Set(startIndex++, cellNodeIds[2]);

      } else if (shape.Id == vtkm::CELL_SHAPE_QUAD) {
        connectivity.Set(startIndex++, cellNodeIds[0]);
        connectivity.Set(startIndex++, cellNodeIds[1]);
        connectivity.Set(startIndex++, cellNodeIds[2]);

        connectivity.Set(startIndex++, cellNodeIds[0]);
        connectivity.Set(startIndex++, cellNodeIds[2]);
        connectivity.Set(startIndex++, cellNodeIds[3]);

      } else if (shape.Id == vtkm::CELL_SHAPE_POLYGON) {
        for (vtkm::Id tri = 0; tri < numIndices-2; tri++) {
          connectivity.Set(startIndex++, cellNodeIds[0]);
          connectivity.Set(startIndex++, cellNodeIds[tri+1]);
          connectivity.Set(startIndex++, cellNodeIds[tri+2]);
        }
      }
    }
  };

  //
  // Construct the filter to tetrahedralize explicit grid
  //
  TetrahedralizeFilterExplicitGrid(const vtkm::cont::DataSet &inDataSet,
                                         vtkm::cont::DataSet &outDataSet) :
    InDataSet(inDataSet),
    OutDataSet(outDataSet)
  {
  }

  vtkm::cont::DataSet InDataSet;  // input dataset with structured cell set
  vtkm::cont::DataSet OutDataSet; // output dataset with explicit cell set

  //
  // Populate the output dataset with triangles based on input explicit dataset
  //
  void Run()
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;

    // Cell sets belonging to input and output datasets
    vtkm::cont::CellSetExplicit<> &inCellSet =
      this->InDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetExplicit<> >();
    vtkm::cont::CellSetExplicit<> &cellSet = 
      this->OutDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetExplicit<> >();

    // Input dataset vertices and cell counts
    vtkm::Id numberOfInCells = inCellSet.GetNumberOfCells();

    // Input topology
    vtkm::cont::ArrayHandle<vtkm::Id> inShapes = inCellSet.GetShapesArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
    vtkm::cont::ArrayHandle<vtkm::Id> inNumIndices = inCellSet.GetNumIndicesArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
    vtkm::cont::ArrayHandle<vtkm::Id> inConn = inCellSet.GetConnectivityArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

    // Cell indices are just counting array
    //vtkm::cont::ArrayHandleCounting<vtkm::Id> cellIndicesArray(0, 1, numberOfInCells);

    // Determine the number of triangles each cell will generate
    vtkm::cont::ArrayHandle<vtkm::Id> trianglesPerCellArray;
    vtkm::worklet::DispatcherMapField<TrianglesPerCell> trianglesPerCellDispatcher;
    trianglesPerCellDispatcher.Invoke(inShapes, inNumIndices, trianglesPerCellArray);

    // Number of triangles and number of vertices needed
    vtkm::cont::ArrayHandle<vtkm::Id> triangleOffset;
    vtkm::Id numberOfOutCells = DeviceAlgorithms::ScanExclusive(trianglesPerCellArray,
                                                                triangleOffset);
    vtkm::Id numberOfOutIndices = numberOfOutCells * 3;

    // Information needed to build the output cell set
    vtkm::cont::ArrayHandle<vtkm::Id> shapes;
    vtkm::cont::ArrayHandle<vtkm::Id> numIndices;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

    shapes.Allocate(static_cast<vtkm::Id>(numberOfOutCells));
    numIndices.Allocate(static_cast<vtkm::Id>(numberOfOutCells));
    connectivity.Allocate(static_cast<vtkm::Id>(numberOfOutIndices));

    // Fill the arrays of shapes and number of indices needed by the cell set
    for (vtkm::Id j = 0; j < numberOfOutCells; j++) {
      shapes.GetPortalControl().Set(j, static_cast<vtkm::Id>(vtkm::CELL_SHAPE_TRIANGLE));
      numIndices.GetPortalControl().Set(j, 3);
    }

    // Call the TriangulateCell functor to compute the triangle connectivity
    vtkm::worklet::DispatcherMapTopology<TriangulateCell> triangulateCellDispatcher;
    triangulateCellDispatcher.Invoke(
                      triangleOffset,
                      inNumIndices,
                      inCellSet,
                      vtkm::exec::ExecutionWholeArray<vtkm::Id>(connectivity, numberOfOutIndices));

    // Add tets to output cellset
    cellSet.Fill(shapes, numIndices, connectivity);
  }

};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_TetrahedralizeExplicitGrid_h
