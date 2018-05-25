//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_TriangulateStructured_h
#define vtk_m_worklet_TriangulateStructured_h

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

namespace vtkm
{
namespace worklet
{
namespace triangulate
{
//
// Worklet to turn quads into triangles
// Vertices remain the same and each cell is processed with needing topology
//
class TriangulateCell : public vtkm::worklet::WorkletMapPointToCell
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldOutCell<> connectivityOut);
  using ExecutionSignature = void(PointIndices, _2, VisitIndex);
  using InputDomain = _1;

  using ScatterType = vtkm::worklet::ScatterUniform<2>;

  // Each quad cell produces 2 triangle cells
  template <typename ConnectivityInVec, typename ConnectivityOutVec>
  VTKM_EXEC void operator()(const ConnectivityInVec& connectivityIn,
                            ConnectivityOutVec& connectivityOut,
                            vtkm::IdComponent visitIndex) const
  {
    VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent StructuredTriangleIndices[2][3] = { { 0, 1, 2 },
                                                                                      { 0, 2, 3 } };
    connectivityOut[0] = connectivityIn[StructuredTriangleIndices[visitIndex][0]];
    connectivityOut[1] = connectivityIn[StructuredTriangleIndices[visitIndex][1]];
    connectivityOut[2] = connectivityIn[StructuredTriangleIndices[visitIndex][2]];
  }
};
}

/// \brief Compute the triangulate cells for a uniform grid data set
template <typename DeviceAdapter>
class TriangulateStructured
{
public:
  template <typename CellSetType>
  vtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      vtkm::cont::ArrayHandle<vtkm::IdComponent>& outCellsPerCell)

  {
    using DeviceAlgorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    vtkm::cont::CellSetSingleType<> outCellSet(cellSet.GetName());
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

    vtkm::worklet::DispatcherMapTopology<triangulate::TriangulateCell, DeviceAdapter> dispatcher;
    dispatcher.Invoke(cellSet, vtkm::cont::make_ArrayHandleGroupVec<3>(connectivity));

    // Fill in array of output cells per input cell
    DeviceAlgorithm::Copy(
      vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(2, cellSet.GetNumberOfCells()),
      outCellsPerCell);

    // Add cells to output cellset
    outCellSet.Fill(cellSet.GetNumberOfPoints(), vtkm::CellShapeTagTriangle::Id, 3, connectivity);
    return outCellSet;
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_TriangulateStructured_h
