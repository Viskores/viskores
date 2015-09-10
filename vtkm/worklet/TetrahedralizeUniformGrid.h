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
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/Pair.h>
#include <vtkm/TopologyElementTag.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/Field.h>
#include <vtkm/VectorAnalysis.h>

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

    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id,4> >::template ExecutionTypes<DeviceAdapter>::Portal TetPortalType;
    TetPortalType Tetrahedra;

    template<typename T>
    VTKM_CONT_EXPORT
    TetrahedralizeCell(const vtkm::Id3 &cdims,
                       const T& tetrahedra) :
      xdim(cdims[0]), ydim(cdims[1]), zdim(cdims[2]),
      cellsPerLayer(xdim * ydim),
      pointsPerLayer((xdim+1) * (ydim+1)),
      Tetrahedra(tetrahedra)
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
printf("CellID %ld vertex pts %ld %ld %ld %ld %ld %ld %ld %ld\n", inputCellId, i0, i1, i2, i3, i4, i5, i6, i7);

      // Set the tetrahedra for this cell based on vertex index and index type of cell
      if (indexType == 0) {
        this->Tetrahedra.Set(inputCellId * 5 + 0, make_Vec<vtkm::Id>(i0, i1, i3, i4));
        this->Tetrahedra.Set(inputCellId * 5 + 1, make_Vec<vtkm::Id>(i1, i4, i5, i6));
        this->Tetrahedra.Set(inputCellId * 5 + 2, make_Vec<vtkm::Id>(i1, i4, i6, i3));
        this->Tetrahedra.Set(inputCellId * 5 + 3, make_Vec<vtkm::Id>(i1, i3, i6, i2));
        this->Tetrahedra.Set(inputCellId * 5 + 4, make_Vec<vtkm::Id>(i3, i6, i7, i4));
      } else {
        this->Tetrahedra.Set(inputCellId * 5 + 0, make_Vec<vtkm::Id>(i2, i1, i5, i0));
        this->Tetrahedra.Set(inputCellId * 5 + 1, make_Vec<vtkm::Id>(i0, i2, i3, i7));
        this->Tetrahedra.Set(inputCellId * 5 + 2, make_Vec<vtkm::Id>(i2, i5, i6, i7));
        this->Tetrahedra.Set(inputCellId * 5 + 3, make_Vec<vtkm::Id>(i0, i7, i4, i4));
        this->Tetrahedra.Set(inputCellId * 5 + 4, make_Vec<vtkm::Id>(i0, i2, i7, i5));
      }

      for (vtkm::Id i = 0; i < 5; i++) {
        vtkm::Vec<vtkm::Id, 4> tet = this->Tetrahedra.Get(inputCellId * 5 + i);
printf("    CellID %ld tet %ld = %ld %ld %ld %ld\n", inputCellId, i, tet[0], tet[1], tet[2], tet[3]);
      }
    }
  };

  TetrahedralizeFilterUniformGrid(const vtkm::Id3 &dims,
                                  const vtkm::cont::DataSet &dataSet,
                                  vtkm::cont::CellSetExplicit<> &cellSet) :
    CDims(dims),
    InDataSet(dataSet),
    OutCellSet(cellSet),
    numberOfPoints((dims[0] + 1) * (dims[1] + 1) * (dims[2] + 1)),
    numberOfInCells(dims[0] * dims[1] * dims[2]),
    numberOfOutCells(5 * numberOfInCells)
  {
  }

  vtkm::Id3 CDims;
  vtkm::cont::DataSet InDataSet;
  vtkm::cont::CellSetExplicit<> OutCellSet;
  vtkm::Id numberOfPoints;
  vtkm::Id numberOfInCells;
  vtkm::Id numberOfOutCells;

  void Run()
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;

    // Cell indices are just counting array
    vtkm::cont::ArrayHandleCounting<vtkm::Id> cellIndicesArray(0, this->numberOfInCells);

    // Output is 5 tets per hex cell
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id,4> > tetrahedra;
    tetrahedra.Allocate(numberOfOutCells);

    // Call the TetrahedralizeCell functor to compute the 5 tets belonging to each hex cell
    TetrahedralizeCell tetrahedralizeCell(
                                     this->CDims, 
                                     tetrahedra.PrepareForOutput(numberOfOutCells, DeviceAdapter()));

    typedef typename vtkm::worklet::DispatcherMapField<TetrahedralizeCell> TetrahedralizeCellDispatcher;
    TetrahedralizeCellDispatcher tetrahedralizeCellDispatcher(tetrahedralizeCell);

    tetrahedralizeCellDispatcher.Invoke(cellIndicesArray);

    // Set up the output data set so that it can received cells of tetrahedra
    this->OutCellSet.PrepareToAddCells(this->numberOfOutCells, this->numberOfOutCells * 4);

    // Add tets to output cellset
    for (vtkm::Id i = 0; i < this->numberOfOutCells; i++) {
      vtkm::Vec<vtkm::Id, 4> tet = tetrahedra.GetPortalControl().Get(i);
      this->OutCellSet.AddCell(vtkm::CELL_SHAPE_TETRA, 4, tetrahedra.GetPortalControl().Get(i));
    }

    // Complete the output data set
    this->OutCellSet.CompleteAddingCells();
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_IsosurfaceUniformGrid_h
