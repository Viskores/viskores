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
    void operator()(const vtkm::UInt8 &shape, 
                    const vtkm::IdComponent &numIndices,
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
  // Worklet to count the number of tetrahedra generated per cell
  //
  class TetrahedraPerCell : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<> shapes,
                                  FieldOut<> triangleCount);
    typedef void ExecutionSignature(_1,_2);
    typedef _1 InputDomain;

    VTKM_CONT_EXPORT
    TetrahedraPerCell() {}
  
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::UInt8 &shape, 
                    vtkm::Id &tetrahedraCount) const
    {
      if (shape == vtkm::CELL_SHAPE_TETRA)
        tetrahedraCount = 1;
      else if (shape == vtkm::CELL_SHAPE_HEXAHEDRON)
        tetrahedraCount = 5;
      else if (shape == vtkm::CELL_SHAPE_WEDGE)
        tetrahedraCount = 3;
      else if (shape == vtkm::CELL_SHAPE_PYRAMID)
        tetrahedraCount = 2;
      else tetrahedraCount = 0;
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
      if (shape.Id == vtkm::CELL_SHAPE_TRIANGLE)
      {
        connectivity.Set(startIndex++, cellNodeIds[0]);
        connectivity.Set(startIndex++, cellNodeIds[1]);
        connectivity.Set(startIndex++, cellNodeIds[2]);
      }
      else if (shape.Id == vtkm::CELL_SHAPE_QUAD)
      {
        connectivity.Set(startIndex++, cellNodeIds[0]);
        connectivity.Set(startIndex++, cellNodeIds[1]);
        connectivity.Set(startIndex++, cellNodeIds[2]);

        connectivity.Set(startIndex++, cellNodeIds[0]);
        connectivity.Set(startIndex++, cellNodeIds[2]);
        connectivity.Set(startIndex++, cellNodeIds[3]);
      }
      else if (shape.Id == vtkm::CELL_SHAPE_POLYGON)
      {
        for (vtkm::IdComponent tri = 0; tri < numIndices-2; tri++)
        {
          connectivity.Set(startIndex++, cellNodeIds[0]);
          connectivity.Set(startIndex++, cellNodeIds[tri+1]);
          connectivity.Set(startIndex++, cellNodeIds[tri+2]);
        }
      }
    }
  };

  //
  // Worklet to turn cells into tetrahedra
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TetrahedralizeCell : public vtkm::worklet::WorkletMapTopologyPointToCell
  {
  public:
    typedef void ControlSignature(FieldInTo<> tetraOffset,
                                  TopologyIn topology,
                                  ExecObject connectivity);
    typedef void ExecutionSignature(_1,_3, CellShape, FromIndices);
    typedef _2 InputDomain;

    VTKM_CONT_EXPORT
    TetrahedralizeCell() {}

    // Each cell produces tetrahedra and write result at the offset
    template<typename CellShapeTag, typename CellNodeVecType>
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &offset,
                    vtkm::exec::ExecutionWholeArray<vtkm::Id> &connectivity,
                    CellShapeTag shape,
                    const CellNodeVecType &cellNodeIds) const
    {
      // Offset is in tetrahedra, 4 vertices per tetrahedron needed
      vtkm::Id startIndex = offset * 4;
      if (shape.Id == vtkm::CELL_SHAPE_TETRA)
      {
        connectivity.Set(startIndex++, cellNodeIds[0]);
        connectivity.Set(startIndex++, cellNodeIds[1]);
        connectivity.Set(startIndex++, cellNodeIds[2]);
        connectivity.Set(startIndex++, cellNodeIds[3]);
      }
      else if (shape.Id == vtkm::CELL_SHAPE_HEXAHEDRON)
      {
        connectivity.Set(startIndex++, cellNodeIds[0]);
        connectivity.Set(startIndex++, cellNodeIds[1]);
        connectivity.Set(startIndex++, cellNodeIds[3]);
        connectivity.Set(startIndex++, cellNodeIds[4]);

        connectivity.Set(startIndex++, cellNodeIds[1]);
        connectivity.Set(startIndex++, cellNodeIds[4]);
        connectivity.Set(startIndex++, cellNodeIds[5]);
        connectivity.Set(startIndex++, cellNodeIds[6]);

        connectivity.Set(startIndex++, cellNodeIds[1]);
        connectivity.Set(startIndex++, cellNodeIds[4]);
        connectivity.Set(startIndex++, cellNodeIds[6]);
        connectivity.Set(startIndex++, cellNodeIds[3]);

        connectivity.Set(startIndex++, cellNodeIds[1]);
        connectivity.Set(startIndex++, cellNodeIds[3]);
        connectivity.Set(startIndex++, cellNodeIds[6]);
        connectivity.Set(startIndex++, cellNodeIds[2]);

        connectivity.Set(startIndex++, cellNodeIds[3]);
        connectivity.Set(startIndex++, cellNodeIds[6]);
        connectivity.Set(startIndex++, cellNodeIds[7]);
        connectivity.Set(startIndex++, cellNodeIds[4]);
      }
      else if (shape.Id == vtkm::CELL_SHAPE_WEDGE)
      {
        connectivity.Set(startIndex++, cellNodeIds[0]);
        connectivity.Set(startIndex++, cellNodeIds[1]);
        connectivity.Set(startIndex++, cellNodeIds[2]);
        connectivity.Set(startIndex++, cellNodeIds[4]);

        connectivity.Set(startIndex++, cellNodeIds[3]);
        connectivity.Set(startIndex++, cellNodeIds[4]);
        connectivity.Set(startIndex++, cellNodeIds[5]);
        connectivity.Set(startIndex++, cellNodeIds[2]);

        connectivity.Set(startIndex++, cellNodeIds[0]);
        connectivity.Set(startIndex++, cellNodeIds[2]);
        connectivity.Set(startIndex++, cellNodeIds[3]);
        connectivity.Set(startIndex++, cellNodeIds[4]);
      }
      else if (shape.Id == vtkm::CELL_SHAPE_PYRAMID)
      {
        connectivity.Set(startIndex++, cellNodeIds[0]);
        connectivity.Set(startIndex++, cellNodeIds[1]);
        connectivity.Set(startIndex++, cellNodeIds[2]);
        connectivity.Set(startIndex++, cellNodeIds[4]);

        connectivity.Set(startIndex++, cellNodeIds[0]);
        connectivity.Set(startIndex++, cellNodeIds[2]);
        connectivity.Set(startIndex++, cellNodeIds[3]);
        connectivity.Set(startIndex++, cellNodeIds[4]);
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
  {}

  vtkm::cont::DataSet InDataSet;  // input dataset with structured cell set
  vtkm::cont::DataSet OutDataSet; // output dataset with explicit cell set

  //
  // Populate the output dataset with triangles or tetrahedra based on input explicit dataset
  //
  void Run()
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;

    // Cell sets belonging to input and output datasets
    vtkm::cont::CellSetExplicit<> &inCellSet =
      InDataSet.GetCellSet(0).template CastTo<vtkm::cont::CellSetExplicit<> >();
    vtkm::cont::CellSetSingleType<> &cellSet = 
      OutDataSet.GetCellSet(0).template CastTo<vtkm::cont::CellSetSingleType<> >();

    // Input dataset vertices and cell counts
    vtkm::Id numberOfInCells = inCellSet.GetNumberOfCells();
    vtkm::Id dimensionality = inCellSet.GetDimensionality();

    // Input topology
    vtkm::cont::ArrayHandle<vtkm::UInt8> inShapes = inCellSet.GetShapesArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
    vtkm::cont::ArrayHandle<vtkm::IdComponent> inNumIndices = inCellSet.GetNumIndicesArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
    vtkm::cont::ArrayHandle<vtkm::Id> inConn = inCellSet.GetConnectivityArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

    // Determine the number of output cells each input cell will generate
    vtkm::cont::ArrayHandle<vtkm::Id> numOutCellArray;
    vtkm::IdComponent verticesPerOutCell = 0;

    if (dimensionality == 2)
    {
      verticesPerOutCell = 3;
      vtkm::worklet::DispatcherMapField<TrianglesPerCell> trianglesPerCellDispatcher;
      trianglesPerCellDispatcher.Invoke(inShapes, inNumIndices, numOutCellArray);
    }
    else if (dimensionality == 3)
    {
      verticesPerOutCell = 4;
      vtkm::worklet::DispatcherMapField<TetrahedraPerCell> tetrahedraPerCellDispatcher;
      tetrahedraPerCellDispatcher.Invoke(inShapes, numOutCellArray);
    }

    // Number of output cells and number of vertices needed
    vtkm::cont::ArrayHandle<vtkm::Id> cellOffset;
    vtkm::Id numberOfOutCells = DeviceAlgorithms::ScanExclusive(numOutCellArray,
                                                                cellOffset);
    vtkm::Id numberOfOutIndices = numberOfOutCells * verticesPerOutCell;

    // Information needed to build the output cell set
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    connectivity.Allocate(numberOfOutIndices);

    // Call worklet to compute the connectivity
    if (dimensionality == 2)
    {
      vtkm::worklet::DispatcherMapTopology<TriangulateCell> triangulateCellDispatcher;
      triangulateCellDispatcher.Invoke(
                      cellOffset,
                      inNumIndices,
                      inCellSet,
                      vtkm::exec::ExecutionWholeArray<vtkm::Id>(connectivity, numberOfOutIndices));
    }
    else if (dimensionality == 3)
    {
      vtkm::worklet::DispatcherMapTopology<TetrahedralizeCell> tetrahedralizeCellDispatcher;
      tetrahedralizeCellDispatcher.Invoke(
                      cellOffset,
                      inCellSet,
                      vtkm::exec::ExecutionWholeArray<vtkm::Id>(connectivity, numberOfOutIndices));
    }
    // Add cells to output cellset
    cellSet.Fill(connectivity);
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_TetrahedralizeExplicitGrid_h
