//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_lcs_GridMetaData_h
#define vtk_m_worklet_lcs_GridMetaData_h

namespace vtkm
{
namespace worklet
{
namespace detail
{

class GridMetaData
{
public:
  using Structured2DType = vtkm::cont::CellSetStructured<2>;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;

  VTKM_CONT
  GridMetaData(const vtkm::cont::DynamicCellSet cellSet)
  {
    if (cellSet.IsType<Structured2DType>())
    {
      this->cellSet2D = true;
      vtkm::Id2 dims =
        cellSet.Cast<Structured2DType>().GetSchedulingRange(vtkm::TopologyElementTagPoint());
      this->Dims = vtkm::Id3(dims[0], dims[1], 1);
    }
    else
    {
      this->cellSet2D = false;
      this->Dims =
        cellSet.Cast<Structured3DType>().GetSchedulingRange(vtkm::TopologyElementTagPoint());
    }
    this->PlaneSize = Dims[0] * Dims[1];
    this->RowSize = Dims[0];
  }

  VTKM_EXEC
  bool IsCellSet2D() const { return this->cellSet2D; }

  VTKM_EXEC
  void GetLogicalIndex(const vtkm::Id index, vtkm::Id3& logicalIndex) const
  {
    logicalIndex[0] = index % Dims[0];
    logicalIndex[1] = (index / Dims[0]) % Dims[1];
    if (this->cellSet2D)
      logicalIndex[2] = 0;
    else
      logicalIndex[2] = index / (Dims[0] * Dims[1]);
  }

  VTKM_EXEC
  const vtkm::Vec<vtkm::Id, 6> GetNeighborIndices(const vtkm::Id index) const
  {
    vtkm::Vec<vtkm::Id, 6> indices;
    vtkm::Id3 logicalIndex;
    GetLogicalIndex(index, logicalIndex);

    // For differentials w.r.t delta in x
    indices[0] = (logicalIndex[0] == 0) ? index : index - 1;
    indices[1] = (logicalIndex[0] == Dims[0] - 1) ? index : index + 1;
    // For differentials w.r.t delta in y
    indices[2] = (logicalIndex[1] == 0) ? index : index - RowSize;
    indices[3] = (logicalIndex[1] == Dims[1] - 1) ? index : index + RowSize;
    if (!this->cellSet2D)
    {
      // For differentials w.r.t delta in z
      indices[4] = (logicalIndex[2] == 0) ? index : index - PlaneSize;
      indices[5] = (logicalIndex[2] == Dims[2] - 1) ? index : index + PlaneSize;
    }
    return indices;
  }

private:
  bool cellSet2D = false;
  vtkm::Id3 Dims;
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;
};

} // namespace detail
} // namespace worklet
} // namespace vtkm

#endif //vtk_m_worklet_lcs_GridMetaData_h
