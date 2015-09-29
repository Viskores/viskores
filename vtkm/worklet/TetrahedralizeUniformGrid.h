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
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CellSetSingleType.h>
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
      if (indexType == 0)
      {
        connectivity.Set(startIndex++, i0);
        connectivity.Set(startIndex++, i1);
        connectivity.Set(startIndex++, i2);

        connectivity.Set(startIndex++, i0);
        connectivity.Set(startIndex++, i2);
        connectivity.Set(startIndex++, i3);
      }
      else
      {
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
      if (indexType == 0)
      {
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
      }
      else
      {
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
  // Populate the output dataset with triangles or tetrahedra based on input uniform dataset
  //
  void Run()
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;

    // Get the cell set from the output data set
    vtkm::cont::CellSetSingleType<> & cellSet =
      OutDataSet.GetCellSet(0).template CastTo<vtkm::cont::CellSetSingleType<> >();

    // Get dimensionality from the explicit cell set
    vtkm::IdComponent dim = cellSet.GetDimensionality();
    vtkm::Id outCellsPerInCell;
    vtkm::IdComponent verticesPerOutCell;
    vtkm::Id numberOfInCells;
    vtkm::Id2 cdims2;
    vtkm::Id3 cdims3;

    // From the uniform dimension get more information
    if (dim == 2)
    {
      outCellsPerInCell = 2;
      verticesPerOutCell = 3;

      vtkm::cont::CellSetStructured<2> &inCellSet = 
        InDataSet.GetCellSet(0).template CastTo<vtkm::cont::CellSetStructured<2> >();
      cdims2 = inCellSet.GetSchedulingRange(vtkm::TopologyElementTagCell());
      numberOfInCells = cdims2[0] * cdims2[1];

    }
    else if (dim == 3)
    {
      outCellsPerInCell = 5;
      verticesPerOutCell = 4;

      vtkm::cont::CellSetStructured<3> &inCellSet = 
        InDataSet.GetCellSet(0).template CastTo<vtkm::cont::CellSetStructured<3> >();
      cdims3 = inCellSet.GetSchedulingRange(vtkm::TopologyElementTagCell());
      numberOfInCells = cdims3[0] * cdims3[1] * cdims3[2];
    }

    vtkm::Id numberOfOutCells = outCellsPerInCell * numberOfInCells;
    vtkm::Id numberOfOutIndices = verticesPerOutCell * numberOfOutCells;

    // Cell indices are just counting array
    vtkm::cont::ArrayHandleCounting<vtkm::Id> cellIndicesArray(0, 1, numberOfInCells);

    // Output dataset depends on dimension and size
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    connectivity.Allocate(numberOfOutIndices);

    // Call the TetrahedralizeCell functor to compute tetrahedra or triangles
    if (dim == 2)
    {
      TriangulateCell triangulateCell(cdims2);
      vtkm::worklet::DispatcherMapField<TriangulateCell> triangulateCellDispatcher(triangulateCell);
      triangulateCellDispatcher.Invoke(
                      cellIndicesArray,
                      vtkm::exec::ExecutionWholeArray<vtkm::Id>(connectivity, numberOfOutIndices));

    }
    else if (dim == 3)
    {
      TetrahedralizeCell tetrahedralizeCell(cdims3);
      vtkm::worklet::DispatcherMapField<TetrahedralizeCell> tetrahedralizeCellDispatcher(tetrahedralizeCell);
      tetrahedralizeCellDispatcher.Invoke(
                      cellIndicesArray,
                      vtkm::exec::ExecutionWholeArray<vtkm::Id>(connectivity, numberOfOutIndices));
    }

    // Add cells to output cellset
    cellSet.Fill(connectivity);
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_TetrahedralizeUniformGrid_h
