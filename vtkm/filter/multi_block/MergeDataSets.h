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
/// \brief Merging multiple data sets into one data set.
///
/// This filter merges multiple data sets into one data set. We assume that the input data sets
/// have the same coordinate system. If there are missing fields in a specific data set,
//  the filter uses the InvalidValue specified by the user to fill in the associated position of the field array.
class VTKM_FILTER_MULTI_BLOCK_EXPORT MergeDataSets : public vtkm::filter::Filter
{
public:
  void SetInvalidValue(vtkm::Float64 invalidValue) { this->InvalidValue = invalidValue; };
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
