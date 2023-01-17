//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_multi_block_AmrArrays_h
#define vtk_m_filter_multi_block_AmrArrays_h

#include <vtkm/cont/ErrorFilterExecution.h>

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/multi_block/vtkm_filter_multi_block_export.h>

namespace vtkm
{
namespace filter
{
namespace multi_block
{
class VTKM_FILTER_MULTI_BLOCK_EXPORT AmrArrays : public vtkm::filter::Filter
{
private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet&) override
  {
    throw vtkm::cont::ErrorFilterExecution("AmrArray only works for a PartitionedDataSet");
  }
  vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& input) override;

  /// the list of ids contains all amrIds of the level above/below that have an overlap
  VTKM_CONT
  void GenerateParentChildInformation();

  /// the corresponding template function based on the dimension of this dataset
  VTKM_CONT
  template <vtkm::IdComponent Dim>
  void ComputeGenerateParentChildInformation();

  /// generate the vtkGhostType array based on the overlap analogously to vtk
  /// blanked cells: 8 normal cells: 0
  VTKM_CONT
  void GenerateGhostType();

  /// the corresponding template function based on the dimension of this dataset
  VTKM_CONT
  template <vtkm::IdComponent Dim>
  void ComputeGenerateGhostType();

  /// Add helper arrays like in ParaView
  VTKM_CONT
  void GenerateIndexArrays();

  /// the input partitioned dataset
  vtkm::cont::PartitionedDataSet AmrDataSet;

  /// per level
  /// contains the partitionIds of each level and blockId
  std::vector<std::vector<vtkm::Id>> PartitionIds;

  /// per partitionId
  /// contains all PartitonIds of the level above that have an overlap
  std::vector<std::vector<vtkm::Id>> ParentsIdsVector;

  /// per partitionId
  /// contains all PartitonIds of the level below that have an overlap
  std::vector<std::vector<vtkm::Id>> ChildrenIdsVector;
};
} // namespace multi_block
} // namesapce filter
} // namespace vtkm

#endif //vtk_m_filter_multi_block_AmrArrays_h
