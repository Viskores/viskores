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

#ifndef vtk_m_worklet_TriangulateUniformGrid_h
#define vtk_m_worklet_TriangulateUniformGrid_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterUniform.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm {
namespace worklet {

namespace detail {

VTKM_EXEC_CONSTANT
const static vtkm::IdComponent StructuredTriangleIndices[2][3] = {
  { 0, 1, 2 },
  { 0, 2, 3 }
};

} // namespace detail

/// \brief Compute the triangulate cells for a uniform grid data set
template <typename DeviceAdapter>
class TriangulateFilterUniformGrid
{
public:

  //
  // Worklet to turn quads into triangles
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TriangulateCell : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  FieldOutCell<> connectivityOut);
    typedef void ExecutionSignature(PointIndices, _2, VisitIndex);
    typedef _1 InputDomain;

    typedef vtkm::worklet::ScatterUniform ScatterType;
    VTKM_CONT
    ScatterType GetScatter() const
    {
      return ScatterType(2);
    }

    VTKM_CONT
    TriangulateCell()
    {  }

    // Each quad cell produces 2 triangle cells
    template<typename ConnectivityInVec, typename ConnectivityOutVec>
    VTKM_EXEC
    void operator()(const ConnectivityInVec &connectivityIn,
                    ConnectivityOutVec &connectivityOut,
                    vtkm::IdComponent visitIndex) const
    {
      connectivityOut[0] = connectivityIn[detail::StructuredTriangleIndices[visitIndex][0]];
      connectivityOut[1] = connectivityIn[detail::StructuredTriangleIndices[visitIndex][1]];
      connectivityOut[2] = connectivityIn[detail::StructuredTriangleIndices[visitIndex][2]];
    }
  };

  //
  // Construct the filter to triangulate uniform grid
  //
  TriangulateFilterUniformGrid(const vtkm::cont::DataSet &inDataSet,
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
  void Run()
  {
    // Get the cell set from the output data set
    vtkm::cont::CellSetSingleType<> &cellSet =
        this->OutDataSet.GetCellSet(0).template Cast<vtkm::cont::CellSetSingleType<> >();

    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

    vtkm::cont::CellSetStructured<2> inCellSet;
    InDataSet.GetCellSet(0).CopyTo(inCellSet);
    vtkm::worklet::DispatcherMapTopology<TriangulateCell,DeviceAdapter> dispatcher;
    dispatcher.Invoke(inCellSet,
                      vtkm::cont::make_ArrayHandleGroupVec<3>(connectivity));

    // Add cells to output cellset
    cellSet.Fill(
          this->OutDataSet.GetCoordinateSystem().GetData().GetNumberOfValues(),
          vtkm::CellShapeTagTriangle::Id,
          3,
          connectivity);
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_TriangulateUniformGrid_h
