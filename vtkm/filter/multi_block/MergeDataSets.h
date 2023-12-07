//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_multi_block_MergeDataSets_h
#define vtk_m_filter_multi_block_MergeDataSets_h

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/Filter.h>
#include <vtkm/filter/multi_block/vtkm_filter_multi_block_export.h>

namespace vtkm
{
namespace filter
{
namespace multi_block
{
/// @brief Merging multiple data sets into one data set.
///
/// This filter merges multiple data sets into one data set. We assume that the input data sets
/// have the same coordinate system. If there are missing fields in a specific data set,
/// the filter uses the InvalidValue specified by the user to fill in the associated position
/// of the field array.
///
/// `MergeDataSets` is used by passing a `vtkm::cont::PartitionedDataSet` to its `Execute()`
/// method. The `Execute()` will return a `vtkm::cont::PartitionedDataSet` because that is
/// the common interface for all filters. However, the `vtkm::cont::PartitionedDataSet` will
/// have one partition that is all the blocks merged together.
class VTKM_FILTER_MULTI_BLOCK_EXPORT MergeDataSets : public vtkm::filter::Filter
{
public:
  /// @brief Specify the value to use where field values are missing.
  ///
  /// One issue when merging blocks in a paritioned dataset is that the blocks/partitions
  /// may have different fields. That is, one partition might not have all the fields of
  /// another partition. When these partitions are merged together, the values for this
  /// missing field must be set to something. They will be set to this value, which defaults
  /// to NaN.
  void SetInvalidValue(vtkm::Float64 invalidValue) { this->InvalidValue = invalidValue; };
  /// @copydoc SetInvalidValue
  vtkm::Float64 GetInvalidValue() { return this->InvalidValue; }

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inputDataSet) override;

  vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& input) override;

  vtkm::Float64 InvalidValue = vtkm::Nan64();
};
} // namespace multi_block
} // namesapce filter
} // namespace vtkm

#endif //vtk_m_filter_multi_block_MergeDataSets_h
