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

#ifndef vtk_m_worklet_TetrahedralizeUniformGrid_h
#define vtk_m_worklet_TetrahedralizeUniformGrid_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterUniform.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm {
namespace worklet {

namespace detail {

VTKM_EXEC_CONSTANT
const static vtkm::IdComponent StructuredTetrahedronIndices[2][5][4] = {
  {
    { 0, 1, 3, 4 },
    { 1, 4, 5, 6 },
    { 1, 4, 6, 3 },
    { 1, 3, 6, 2 },
    { 3, 6, 7, 4 }
  },
  {
    { 2, 1, 5, 0 },
    { 0, 2, 3, 7 },
    { 2, 5, 6, 7 },
    { 0, 7, 4, 5 },
    { 0, 2, 7, 5 }
  }
};

} // namespace detail

/// \brief Compute the tetrahedralize cells for a uniform grid data set
template <typename DeviceAdapter>
class TetrahedralizeFilterUniformGrid
{
public:

  //
  // Worklet to turn hexahedra into tetrahedra
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TetrahedralizeCell : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  FieldOutCell<> connectivityOut);
    typedef void ExecutionSignature(PointIndices, _2, ThreadIndices);
    typedef _1 InputDomain;

    typedef vtkm::worklet::ScatterUniform ScatterType;
    VTKM_CONT
    ScatterType GetScatter() const
    {
      return ScatterType(5);
    }

    VTKM_CONT
    TetrahedralizeCell()
    {  }

    // Each hexahedron cell produces five tetrahedron cells
    template<typename ConnectivityInVec,
             typename ConnectivityOutVec,
             typename ThreadIndicesType>
    VTKM_EXEC
    void operator()(const ConnectivityInVec &connectivityIn,
                    ConnectivityOutVec &connectivityOut,
                    const ThreadIndicesType threadIndices) const
    {
      vtkm::Id3 inputIndex = threadIndices.GetInputIndex3D();

      // Calculate the type of tetrahedron generated because it alternates
      vtkm::Id indexType = (inputIndex[0] + inputIndex[1] + inputIndex[2]) % 2;

      vtkm::IdComponent visitIndex = threadIndices.GetVisitIndex();

      connectivityOut[0] = connectivityIn[detail::StructuredTetrahedronIndices[indexType][visitIndex][0]];
      connectivityOut[1] = connectivityIn[detail::StructuredTetrahedronIndices[indexType][visitIndex][1]];
      connectivityOut[2] = connectivityIn[detail::StructuredTetrahedronIndices[indexType][visitIndex][2]];
      connectivityOut[3] = connectivityIn[detail::StructuredTetrahedronIndices[indexType][visitIndex][3]];
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
  // Populate the output dataset with tetrahedra based on input uniform dataset
  //
  void Run()
  {
    // Get the cell set from the output data set
    vtkm::cont::CellSetSingleType<> &cellSet =
        this->OutDataSet.GetCellSet(0).template Cast<vtkm::cont::CellSetSingleType<> >();

    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

    vtkm::cont::CellSetStructured<3> inCellSet;
    InDataSet.GetCellSet(0).CopyTo(inCellSet);
    vtkm::worklet::DispatcherMapTopology<TetrahedralizeCell,DeviceAdapter> dispatcher;
    dispatcher.Invoke(inCellSet,
                      vtkm::cont::make_ArrayHandleGroupVec<4>(connectivity));

    // Add cells to output cellset
    cellSet.Fill(
          this->OutDataSet.GetCoordinateSystem().GetData().GetNumberOfValues(),
          vtkm::CellShapeTagTetra::Id,
          4,
          connectivity);
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_TetrahedralizeUniformGrid_h
