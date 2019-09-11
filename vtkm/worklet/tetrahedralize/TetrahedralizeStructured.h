//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_TetrahedralizeStructured_h
#define vtk_m_worklet_TetrahedralizeStructured_h

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterUniform.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace worklet
{

namespace tetrahedralize
{
//
// Worklet to turn hexahedra into tetrahedra
// Vertices remain the same and each cell is processed with needing topology
//
class TetrahedralizeCell : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldOutCell connectivityOut);
  using ExecutionSignature = void(PointIndices, _2, ThreadIndices);
  using InputDomain = _1;

  using ScatterType = vtkm::worklet::ScatterUniform<5>;

  // Each hexahedron cell produces five tetrahedron cells
  template <typename ConnectivityInVec, typename ConnectivityOutVec, typename ThreadIndicesType>
  VTKM_EXEC void operator()(const ConnectivityInVec& connectivityIn,
                            ConnectivityOutVec& connectivityOut,
                            const ThreadIndicesType threadIndices) const
  {
    VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent StructuredTetrahedronIndices[2][5][4] = {
      { { 0, 1, 3, 4 }, { 1, 4, 5, 6 }, { 1, 4, 6, 3 }, { 1, 3, 6, 2 }, { 3, 6, 7, 4 } },
      { { 2, 1, 5, 0 }, { 0, 2, 3, 7 }, { 2, 5, 6, 7 }, { 0, 7, 4, 5 }, { 0, 2, 7, 5 } }
    };

    vtkm::Id3 inputIndex = threadIndices.GetInputIndex3D();

    // Calculate the type of tetrahedron generated because it alternates
    vtkm::Id indexType = (inputIndex[0] + inputIndex[1] + inputIndex[2]) % 2;

    vtkm::IdComponent visitIndex = threadIndices.GetVisitIndex();

    connectivityOut[0] = connectivityIn[StructuredTetrahedronIndices[indexType][visitIndex][0]];
    connectivityOut[1] = connectivityIn[StructuredTetrahedronIndices[indexType][visitIndex][1]];
    connectivityOut[2] = connectivityIn[StructuredTetrahedronIndices[indexType][visitIndex][2]];
    connectivityOut[3] = connectivityIn[StructuredTetrahedronIndices[indexType][visitIndex][3]];
  }
};
}

/// \brief Compute the tetrahedralize cells for a uniform grid data set
class TetrahedralizeStructured
{
public:
  template <typename CellSetType>
  vtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      vtkm::cont::ArrayHandle<vtkm::IdComponent>& outCellsPerCell)
  {
    vtkm::cont::CellSetSingleType<> outCellSet;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

    vtkm::worklet::DispatcherMapTopology<tetrahedralize::TetrahedralizeCell> dispatcher;
    dispatcher.Invoke(cellSet, vtkm::cont::make_ArrayHandleGroupVec<4>(connectivity));

    // Fill in array of output cells per input cell
    vtkm::cont::ArrayCopy(
      vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(5, cellSet.GetNumberOfCells()),
      outCellsPerCell);

    // Add cells to output cellset
    outCellSet.Fill(cellSet.GetNumberOfPoints(), vtkm::CellShapeTagTetra::Id, 4, connectivity);
    return outCellSet;
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_TetrahedralizeStructured_h
