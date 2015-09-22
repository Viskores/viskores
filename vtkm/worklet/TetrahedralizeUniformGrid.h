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

#ifndef vtk_m_worklet_TetrahedralizeUniformGrid_h
#define vtk_m_worklet_TetrahedralizeUniformGrid_h

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/exec/ExecutionWholeArray.h>

namespace vtkm {
namespace worklet {

/// \brief Compute the tetrahedralize cells for a uniform grid data set
template <typename DeviceAdapter>
class TetrahedralizeFilterUniformGrid
{
public:

  //
  // Worklet to turn quads into triangles
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TriangulateCell : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType> inputCellId,
                                  ExecObject connectivity);
    typedef void ExecutionSignature(_1,_2);
    typedef _1 InputDomain;

    vtkm::Id xdim, ydim;

    VTKM_CONT_EXPORT
    TriangulateCell(const vtkm::Id2 &cdims) :
      xdim(cdims[0]), ydim(cdims[1])
    {
    }

    // Each hexahedron cell produces five tetrahedron cells
    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id &inputCellId,
                    vtkm::exec::ExecutionWholeArray<vtkm::Id> &connectivity) const
    {
      // Calculate the i,j indices for this input cell id
      const vtkm::Id x = inputCellId % xdim;
      const vtkm::Id y = (inputCellId / xdim) % ydim;

      // Calculate the type of triangle generated because it alternates
      vtkm::Id indexType = (x + y) % 2;

      // Compute indices for the four vertices of this cell
      const vtkm::Id i0 = x    + y*(xdim+1);
      const vtkm::Id i1 = i0   + 1;
      const vtkm::Id i2 = i0   + 1 + (xdim + 1); //xdim is cell dim
      const vtkm::Id i3 = i0   + (xdim + 1);     //xdim is cell dim

      // Set the triangles for this cell based on vertex index and index type of cell
      // 2 triangles per quad, 3 indices per triangle
      vtkm::Id startIndex = inputCellId * 2 * 3;
      if (indexType == 0) {
        connectivity.Set(startIndex++, i0);
        connectivity.Set(startIndex++, i1);
        connectivity.Set(startIndex++, i2);

        connectivity.Set(startIndex++, i0);
        connectivity.Set(startIndex++, i2);
        connectivity.Set(startIndex++, i3);
      } else {
        connectivity.Set(startIndex++, i0);
        connectivity.Set(startIndex++, i1);
        connectivity.Set(startIndex++, i3);

        connectivity.Set(startIndex++, i1);
        connectivity.Set(startIndex++, i2);
        connectivity.Set(startIndex++, i3);
      }
    }
  };

  //
  // Worklet to turn hexahedra into tetrahedra
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TetrahedralizeCell : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType> inputCellId,
                                  ExecObject connectivity);
    typedef void ExecutionSignature(_1,_2);
    typedef _1 InputDomain;

    vtkm::Id xdim, ydim, zdim;
    const vtkm::Id cellsPerLayer, pointsPerLayer;

    VTKM_CONT_EXPORT
    TetrahedralizeCell(const vtkm::Id3 &cdims) :
      xdim(cdims[0]), ydim(cdims[1]), zdim(cdims[2]),
      cellsPerLayer(xdim * ydim),
      pointsPerLayer((xdim+1) * (ydim+1))
    {
    }

    // Each hexahedron cell produces five tetrahedron cells
    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id &inputCellId,
                    vtkm::exec::ExecutionWholeArray<vtkm::Id> &connectivity) const
    {
      // Calculate the i,j,k indices for this input cell id
      const vtkm::Id x = inputCellId % xdim;
      const vtkm::Id y = (inputCellId / xdim) % ydim;
      const vtkm::Id z = inputCellId / cellsPerLayer;

      // Calculate the type of tetrahedron generated because it alternates
      vtkm::Id indexType = (x + y + z) % 2;

      // Compute indices for the eight vertices of this cell
      const vtkm::Id i0 = x    + y*(xdim+1) + z * pointsPerLayer;
      const vtkm::Id i1 = i0   + 1;
      const vtkm::Id i2 = i0   + 1 + (xdim + 1); //xdim is cell dim
      const vtkm::Id i3 = i0   + (xdim + 1);     //xdim is cell dim
      const vtkm::Id i4 = i0   + pointsPerLayer;
      const vtkm::Id i5 = i1   + pointsPerLayer;
      const vtkm::Id i6 = i2   + pointsPerLayer;
      const vtkm::Id i7 = i3   + pointsPerLayer;

      // Set the tetrahedra for this cell based on vertex index and index type of cell
      // 5 tetrahedra per hexahedron, 4 indices per tetrahedron
      vtkm::Id startIndex = inputCellId * 5 * 4;
      if (indexType == 0) {
        connectivity.Set(startIndex++, i0);
        connectivity.Set(startIndex++, i1);
        connectivity.Set(startIndex++, i3);
        connectivity.Set(startIndex++, i4);

        connectivity.Set(startIndex++, i1);
        connectivity.Set(startIndex++, i4);
        connectivity.Set(startIndex++, i5);
        connectivity.Set(startIndex++, i6);

        connectivity.Set(startIndex++, i1);
        connectivity.Set(startIndex++, i4);
        connectivity.Set(startIndex++, i6);
        connectivity.Set(startIndex++, i3);

        connectivity.Set(startIndex++, i1);
        connectivity.Set(startIndex++, i3);
        connectivity.Set(startIndex++, i6);
        connectivity.Set(startIndex++, i2);

        connectivity.Set(startIndex++, i3);
        connectivity.Set(startIndex++, i6);
        connectivity.Set(startIndex++, i7);
        connectivity.Set(startIndex++, i4);
      } else {
        connectivity.Set(startIndex++, i2);
        connectivity.Set(startIndex++, i1);
        connectivity.Set(startIndex++, i5);
        connectivity.Set(startIndex++, i0);

        connectivity.Set(startIndex++, i0);
        connectivity.Set(startIndex++, i2);
        connectivity.Set(startIndex++, i3);
        connectivity.Set(startIndex++, i7);

        connectivity.Set(startIndex++, i2);
        connectivity.Set(startIndex++, i5);
        connectivity.Set(startIndex++, i6);
        connectivity.Set(startIndex++, i7);

        connectivity.Set(startIndex++, i0);
        connectivity.Set(startIndex++, i7);
        connectivity.Set(startIndex++, i4);
        connectivity.Set(startIndex++, i5);

        connectivity.Set(startIndex++, i0);
        connectivity.Set(startIndex++, i2);
        connectivity.Set(startIndex++, i7);
        connectivity.Set(startIndex++, i5);
      }
    }
  };

  //
  // Construct the filter to tetrahedralize uniform grid
  //
  TetrahedralizeFilterUniformGrid(const vtkm::cont::DataSet &inDataSet,
                                  vtkm::cont::DataSet &outDataSet) :
    InDataSet(inDataSet),
    OutDataSet(outDataSet)
  {
  }

  vtkm::cont::DataSet InDataSet;  // input dataset with structured cell set
  vtkm::cont::DataSet OutDataSet; // output dataset with explicit cell set

  //
  // Populate the output dataset with triangles based on input uniform dataset
  //
  void Run(const vtkm::Id2 &cdims)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;

    vtkm::Id numberOfVertices = (cdims[0] + 1) * (cdims[1] + 1);
    vtkm::Id numberOfInCells = cdims[0] * cdims[1];
    vtkm::Id numberOfOutCells = 2 * numberOfInCells;
    vtkm::Id numberOfOutIndices = 3 * numberOfOutCells;

    // Get the cell set from the output data set
    vtkm::cont::CellSetExplicit<> &cellSet = 
      this->OutDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetExplicit<> >();

    // Cell indices are just counting array
    vtkm::cont::ArrayHandleCounting<vtkm::Id> cellIndicesArray(0, 1, numberOfInCells);

    // Output is 5 tets per hex cell so allocate accordingly
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

    // Call the TriangulateCell functor to compute the 2 triangles for connectivity
    TriangulateCell triangulateCell(cdims);
    typedef typename vtkm::worklet::DispatcherMapField<TriangulateCell> TriangulateCellDispatcher;
    TriangulateCellDispatcher triangulateCellDispatcher(triangulateCell);
    triangulateCellDispatcher.Invoke(
                      cellIndicesArray,
                      vtkm::exec::ExecutionWholeArray<vtkm::Id>(connectivity, numberOfOutIndices));

    // Add tets to output cellset
    cellSet.Fill(shapes, numIndices, connectivity);
  }

  //
  // Populate the output dataset with tetrahedra based on input uniform dataset
  //
  void Run(const vtkm::Id3 &cdims)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;

    vtkm::Id numberOfVertices = (cdims[0] + 1) * (cdims[1] + 1) * (cdims[2] + 1);
    vtkm::Id numberOfInCells = cdims[0] * cdims[1] * cdims[2];
    vtkm::Id numberOfOutCells = 5 * numberOfInCells;
    vtkm::Id numberOfOutIndices = 4 * numberOfOutCells;

    // Get the cell set from the output data set
    vtkm::cont::CellSetExplicit<> &cellSet = 
      this->OutDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetExplicit<> >();

    // Cell indices are just counting array
    vtkm::cont::ArrayHandleCounting<vtkm::Id> cellIndicesArray(0, 1, numberOfInCells);

    // Output is 5 tets per hex cell so allocate accordingly
    vtkm::cont::ArrayHandle<vtkm::Id> shapes;
    vtkm::cont::ArrayHandle<vtkm::Id> numIndices;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

    shapes.Allocate(static_cast<vtkm::Id>(numberOfOutCells));
    numIndices.Allocate(static_cast<vtkm::Id>(numberOfOutCells));
    connectivity.Allocate(static_cast<vtkm::Id>(numberOfOutIndices));

    // Fill the arrays of shapes and number of indices needed by the cell set
    for (vtkm::Id j = 0; j < numberOfOutCells; j++) {
      shapes.GetPortalControl().Set(j, static_cast<vtkm::Id>(vtkm::CELL_SHAPE_TETRA));
      numIndices.GetPortalControl().Set(j, 4);
    }

    // Call the TetrahedralizeCell functor to compute the 5 tets for connectivity
    TetrahedralizeCell tetrahedralizeCell(cdims);
    typedef typename vtkm::worklet::DispatcherMapField<TetrahedralizeCell> TetrahedralizeCellDispatcher;
    TetrahedralizeCellDispatcher tetrahedralizeCellDispatcher(tetrahedralizeCell);
    tetrahedralizeCellDispatcher.Invoke(
                      cellIndicesArray,
                      vtkm::exec::ExecutionWholeArray<vtkm::Id>(connectivity, numberOfOutIndices));

    // Add tets to output cellset
    cellSet.Fill(shapes, numIndices, connectivity);
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_TetrahedralizeUniformGrid_h
