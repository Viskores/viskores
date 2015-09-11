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
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/Field.h>

#include "stdio.h"

namespace vtkm {
namespace worklet {

/// \brief Compute the tetrahedralize cells for a uniform grid data set
template <typename DeviceAdapter>
class TetrahedralizeFilterUniformGrid
{
public:

  class TetrahedralizeCell : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType> inputCellId);
    typedef void ExecutionSignature(_1);
    typedef _1 InputDomain;

    vtkm::Id xdim, ydim, zdim;
    const vtkm::Id cellsPerLayer, pointsPerLayer;

    typedef typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapter>::Portal TetPortalType;
    TetPortalType TetrahedraIndices;

    template<typename T>
    VTKM_CONT_EXPORT
    TetrahedralizeCell(const vtkm::Id3 &cdims,
                       const T& tetrahedraIndices) :
      xdim(cdims[0]), ydim(cdims[1]), zdim(cdims[2]),
      cellsPerLayer(xdim * ydim),
      pointsPerLayer((xdim+1) * (ydim+1)),
      TetrahedraIndices(tetrahedraIndices)
    {
    }

    // Each hexahedron cell produces five tetrahedron cells
    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id &inputCellId) const
    {
      // Calculate the i,j,k indices for this input cell id
      const vtkm::Id x = inputCellId % xdim;
      const vtkm::Id y = (inputCellId / xdim) % ydim;
      const vtkm::Id z = inputCellId / cellsPerLayer;

      // Calculate the type of tetrahedron generated because it alternates
      vtkm::Id indexType = (x + y + z) % 2;

printf("CellID %ld x %ld y %ld z %ld indexType %ld\n", inputCellId, x, y, z, indexType);

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
      vtkm::Id startIndex = inputCellId * 5 * 4;
printf("CellID %ld vertex pts %ld %ld %ld %ld %ld %ld %ld %ld STARTINDEX %ld\n", inputCellId, i0, i1, i2, i3, i4, i5, i6, i7, startIndex);
      if (indexType == 0) {
        this->TetrahedraIndices.Set(startIndex + 0, i0);
        this->TetrahedraIndices.Set(startIndex + 1, i1);
        this->TetrahedraIndices.Set(startIndex + 2, i3);
        this->TetrahedraIndices.Set(startIndex + 3, i4);

        startIndex += 4;
        this->TetrahedraIndices.Set(startIndex + 0, i1);
        this->TetrahedraIndices.Set(startIndex + 1, i4);
        this->TetrahedraIndices.Set(startIndex + 2, i5);
        this->TetrahedraIndices.Set(startIndex + 3, i6);

        startIndex += 4;
        this->TetrahedraIndices.Set(startIndex + 0, i1);
        this->TetrahedraIndices.Set(startIndex + 1, i4);
        this->TetrahedraIndices.Set(startIndex + 2, i6);
        this->TetrahedraIndices.Set(startIndex + 3, i3);

        startIndex += 4;
        this->TetrahedraIndices.Set(startIndex + 0, i1);
        this->TetrahedraIndices.Set(startIndex + 1, i3);
        this->TetrahedraIndices.Set(startIndex + 2, i6);
        this->TetrahedraIndices.Set(startIndex + 3, i2);

        startIndex += 4;
        this->TetrahedraIndices.Set(startIndex + 0, i3);
        this->TetrahedraIndices.Set(startIndex + 1, i6);
        this->TetrahedraIndices.Set(startIndex + 2, i7);
        this->TetrahedraIndices.Set(startIndex + 3, i4);

      } else {

        this->TetrahedraIndices.Set(startIndex + 0, i2);
        this->TetrahedraIndices.Set(startIndex + 1, i1);
        this->TetrahedraIndices.Set(startIndex + 2, i5);
        this->TetrahedraIndices.Set(startIndex + 3, i0);

        startIndex += 4;
        this->TetrahedraIndices.Set(startIndex + 0, i0);
        this->TetrahedraIndices.Set(startIndex + 1, i2);
        this->TetrahedraIndices.Set(startIndex + 2, i3);
        this->TetrahedraIndices.Set(startIndex + 3, i7);

        startIndex += 4;
        this->TetrahedraIndices.Set(startIndex + 0, i2);
        this->TetrahedraIndices.Set(startIndex + 1, i5);
        this->TetrahedraIndices.Set(startIndex + 2, i6);
        this->TetrahedraIndices.Set(startIndex + 3, i7);

        startIndex += 4;
        this->TetrahedraIndices.Set(startIndex + 0, i0);
        this->TetrahedraIndices.Set(startIndex + 1, i7);
        this->TetrahedraIndices.Set(startIndex + 2, i4);
        this->TetrahedraIndices.Set(startIndex + 3, i5);

        startIndex += 4;
        this->TetrahedraIndices.Set(startIndex + 0, i0);
        this->TetrahedraIndices.Set(startIndex + 1, i2);
        this->TetrahedraIndices.Set(startIndex + 2, i7);
        this->TetrahedraIndices.Set(startIndex + 3, i5);
      }
    }
  };

  TetrahedralizeFilterUniformGrid(const vtkm::Id3 &cdims,
                                  const vtkm::cont::DataSet &inDataSet,
                                  vtkm::cont::DataSet &outDataSet) :
    CDims(cdims),
    InDataSet(inDataSet),
    OutDataSet(outDataSet),
    numberOfVertices((cdims[0] + 1) * (cdims[1] + 1) * (cdims[2] + 1)),
    numberOfInCells(cdims[0] * cdims[1] * cdims[2]),
    numberOfOutCells(5 * numberOfInCells)
  {
  }

  vtkm::Id3 CDims;
  vtkm::cont::DataSet InDataSet;
  vtkm::cont::DataSet OutDataSet;
  vtkm::Id numberOfVertices;
  vtkm::Id numberOfInCells;
  vtkm::Id numberOfOutCells;

  void Run()
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;

    // Get the cell set from the output data set
    vtkm::cont::CellSetExplicit<> &cellSet = 
      this->OutDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetExplicit<> >();

    // Cell indices are just counting array
    vtkm::cont::ArrayHandleCounting<vtkm::Id> cellIndicesArray(0, this->numberOfInCells);

    // Output is 5 tets per hex cell
    vtkm::cont::ArrayHandle<vtkm::Id> shapes;
    vtkm::cont::ArrayHandle<vtkm::Id> numIndices;
    vtkm::cont::ArrayHandle<vtkm::Id> conn;
    shapes.Allocate(static_cast<vtkm::Id>(numberOfOutCells));
    numIndices.Allocate(static_cast<vtkm::Id>(numberOfOutCells));
    conn.Allocate(static_cast<vtkm::Id>(4 * numberOfOutCells));

    for (vtkm::Id j = 0; j < numberOfOutCells; j++) {
      shapes.GetPortalControl().Set(j, static_cast<vtkm::Id>(vtkm::CELL_SHAPE_TETRA));
      numIndices.GetPortalControl().Set(j, 4);
    }

    // Call the TetrahedralizeCell functor to compute the 5 tets belonging to each hex cell
    TetrahedralizeCell tetrahedralizeCell(
                            this->CDims, 
                            conn.PrepareForOutput(numberOfOutCells * 4, DeviceAdapter()));

    typedef typename vtkm::worklet::DispatcherMapField<TetrahedralizeCell> TetrahedralizeCellDispatcher;
    TetrahedralizeCellDispatcher tetrahedralizeCellDispatcher(tetrahedralizeCell);

    tetrahedralizeCellDispatcher.Invoke(cellIndicesArray);

    // Add tets to output cellset
    cellSet.Fill(shapes, numIndices, conn);

    vtkm::Id index = 0;
    for (vtkm::Id j = 0; j < this->numberOfOutCells; j++) {
      printf("Cell %ld Shape %ld NumIndices %ld Tet (%ld, %ld, %ld, %ld)\n",
             j, shapes.GetPortalControl().Get(j), numIndices.GetPortalControl().Get(j),
             conn.GetPortalControl().Get(index++), conn.GetPortalControl().Get(index++),
             conn.GetPortalControl().Get(index++), conn.GetPortalControl().Get(index++));
    }
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_TetrahedralizeUniformGrid_h
