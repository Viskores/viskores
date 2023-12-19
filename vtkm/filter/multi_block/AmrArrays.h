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

/// @brief Generate arrays describing the AMR structure in a partitioned data set.
///
/// AMR grids are represented by `vtkm::cont::PartitionedDataSet`, but this class
/// does not explicitly store the hierarchical structure of the mesh refinement.
/// This hierarchical arrangement needs to be captured in fields that describe
/// where blocks reside in the hierarchy. This filter analyses the arrangement
/// of partitions in a `vtkm::cont::PartitionedDataSet` and generates the following
/// field arrays.
///
/// - `vtkAmrLevel` The AMR level at which the partition resides (with 0 being the
///   most coarse level). All the values for a particular partition are set to the
///   same value.
/// - `vtkAmrIndex` A unique identifier for each partition of a particular level.
///   Each partition of the same level will have a unique index, but the indices
///   will repeat across levels. All the values for a particular partition are set
///   to the same value.
/// - `vtkCompositeIndex` A unique identifier for each partition. This index is the
///   same as the index used for the partition in the containing
///   `vtkm::cont::PartitionedDataSet`. All the values for a particular partition are
///   set to the same value.
/// - `vtkGhostType` It is common for refinement levels in an AMR structure to
///   overlap more coarse grids. In this case, the overlapped coarse cells have
///   invalid data. The vtkGhostType field will track which cells are overlapped
///   and should be ignored. This array will have a 0 value for all valid cells and
///   a non-zero value for all invalid cells. (Specifically, if the bit specified by
///   `vtkm::CellClassification::BLANKED` is set, then the cell is overlapped with a
///   cell in a finer level.)
///
/// These arrays are stored as cell fields in the partitions.
///
/// This filter only operates on partitioned data sets where all the partitions
/// have cell sets of type `vtkm::cont::CellSetStructured`. This is characteristic
/// of AMR data sets.
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

  /// generate the GhostType array based on the overlap analogously to vtk
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
