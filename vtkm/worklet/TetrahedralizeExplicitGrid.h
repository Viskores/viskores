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
    typedef _3 ExecutionSignature(_1,_2);
    typedef _1 InputDomain;

    VTKM_CONT_EXPORT
    TrianglesPerCell() {}

    VTKM_EXEC_EXPORT
    vtkm::IdComponent operator()(vtkm::UInt8 shape,
                                 vtkm::IdComponent numIndices) const
    {
      switch (shape)
      {
        case vtkm::CELL_SHAPE_TRIANGLE: return 1;
        case vtkm::CELL_SHAPE_QUAD:     return 2;
        case vtkm::CELL_SHAPE_POLYGON:  return numIndices - 2;
        default: return 0;
      }
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
    typedef _2 ExecutionSignature(_1);
    typedef _1 InputDomain;

    VTKM_CONT_EXPORT
    TetrahedraPerCell() {}

    VTKM_EXEC_EXPORT
    vtkm::IdComponent operator()(vtkm::UInt8 shape) const
    {
      switch (shape)
      {
        case vtkm::CELL_SHAPE_TETRA: return 1;
        case vtkm::CELL_SHAPE_HEXAHEDRON: return 5;
        case vtkm::CELL_SHAPE_WEDGE: return 3;
        case vtkm::CELL_SHAPE_PYRAMID: return 2;
        default: return 0;
      }
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
                                  FieldOutCell<> connectivityOut);
    typedef void ExecutionSignature(CellShape, PointIndices, _2, VisitIndex);
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
    void operator()(CellShapeTag shape,
                    const ConnectivityInVec &connectivityIn,
                    ConnectivityOutVec &connectivityOut,
                    vtkm::IdComponent visitIndex) const
    {
      if (shape.Id == vtkm::CELL_SHAPE_TRIANGLE)
      {
        connectivityOut[0] = connectivityIn[0];
        connectivityOut[1] = connectivityIn[1];
        connectivityOut[2] = connectivityIn[2];
      }
      else if (shape.Id == vtkm::CELL_SHAPE_QUAD)
      {
        const static vtkm::IdComponent triIndices[2][3] = {
          { 0, 1, 2 },
          { 0, 2, 3 }
        };

        connectivityOut[0] = connectivityIn[triIndices[visitIndex][0]];
        connectivityOut[1] = connectivityIn[triIndices[visitIndex][1]];
        connectivityOut[2] = connectivityIn[triIndices[visitIndex][2]];
      }
      else if (shape.Id == vtkm::CELL_SHAPE_POLYGON)
      {
        connectivityOut[0] = connectivityIn[0];
        connectivityOut[1] = connectivityIn[visitIndex+1];
        connectivityOut[2] = connectivityIn[visitIndex+2];
      }
      else
      {
        this->RaiseError("Invalid cell in triangulate.");
      }
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
                                  FieldOutCell<> connectivityOut);
    typedef void ExecutionSignature(CellShape, PointIndices, _2, VisitIndex);
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
                    ConnectivityOutVec &connectivityOut,
                    vtkm::IdComponent visitIndex) const
    {
      if (shape.Id == vtkm::CELL_SHAPE_TRIANGLE)
      {
        connectivityOut[0] = connectivityIn[0];
        connectivityOut[1] = connectivityIn[1];
        connectivityOut[2] = connectivityIn[2];
        connectivityOut[3] = connectivityIn[3];
      }
      else if (shape.Id == vtkm::CELL_SHAPE_HEXAHEDRON)
      {
        const static vtkm::IdComponent tetIndices[5][4] = {
          { 0, 1, 3, 4 },
          { 1, 4, 5, 6 },
          { 1, 4, 6, 3 },
          { 1, 3, 6, 2 },
          { 3, 6, 7, 4 }
        };

        connectivityOut[0] = connectivityIn[tetIndices[visitIndex][0]];
        connectivityOut[1] = connectivityIn[tetIndices[visitIndex][1]];
        connectivityOut[2] = connectivityIn[tetIndices[visitIndex][2]];
        connectivityOut[3] = connectivityIn[tetIndices[visitIndex][3]];
      }
      else if (shape.Id == vtkm::CELL_SHAPE_WEDGE)
      {
        const static vtkm::IdComponent tetIndices[3][4] = {
          { 0, 1, 2, 4 },
          { 3, 4, 5, 2 },
          { 0, 2, 3, 4 }
        };

        connectivityOut[0] = connectivityIn[tetIndices[visitIndex][0]];
        connectivityOut[1] = connectivityIn[tetIndices[visitIndex][1]];
        connectivityOut[2] = connectivityIn[tetIndices[visitIndex][2]];
        connectivityOut[3] = connectivityIn[tetIndices[visitIndex][3]];
      }
      else if (shape.Id == vtkm::CELL_SHAPE_PYRAMID)
      {
        const static vtkm::IdComponent tetIndices[2][4] = {
          { 0, 1, 2, 4 },
          { 0, 2, 3, 4 }
        };

        connectivityOut[0] = connectivityIn[tetIndices[visitIndex][0]];
        connectivityOut[1] = connectivityIn[tetIndices[visitIndex][1]];
        connectivityOut[2] = connectivityIn[tetIndices[visitIndex][2]];
        connectivityOut[3] = connectivityIn[tetIndices[visitIndex][3]];
      }
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
    vtkm::cont::CellSetExplicit<> &inCellSet =
      InDataSet.GetCellSet(0).template CastTo<vtkm::cont::CellSetExplicit<> >();
    vtkm::cont::CellSetSingleType<> &cellSet =
      OutDataSet.GetCellSet(0).template CastTo<vtkm::cont::CellSetSingleType<> >();

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
      // Determine the number of output cells each input cell will generate
      vtkm::cont::ArrayHandle<vtkm::IdComponent> numOutCellArray;
      vtkm::worklet::DispatcherMapField<TrianglesPerCell,DeviceAdapter>
          triPerCellDispatcher;
      triPerCellDispatcher.Invoke(inShapes, inNumIndices, numOutCellArray);

      // Build new cells
      TriangulateCell triangulateWorklet(numOutCellArray);
      vtkm::worklet::DispatcherMapTopology<TriangulateCell,DeviceAdapter>
          triangulateDispatcher(triangulateWorklet);
      triangulateDispatcher.Invoke(
            inCellSet,
            vtkm::cont::make_ArrayHandleGroupVec<3>(outConnectivity));
    }
    else if (dimensionality == 3)
    {
      // Determine the number of output cells each input cell will generate
      vtkm::cont::ArrayHandle<vtkm::IdComponent> numOutCellArray;
      vtkm::worklet::DispatcherMapField<TetrahedraPerCell,DeviceAdapter>
          tetPerCellDispatcher;
      tetPerCellDispatcher.Invoke(inShapes, numOutCellArray);

      // Build new cells
      TetrahedralizeCell tetrahedralizeWorklet(numOutCellArray);
      vtkm::worklet::DispatcherMapTopology<TetrahedralizeCell,DeviceAdapter>
          tetrahedralizeDispatcher(tetrahedralizeWorklet);
      tetrahedralizeDispatcher.Invoke(
            inCellSet,
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
