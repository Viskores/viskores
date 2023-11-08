//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/MergePartitionedDataSet.h>
#include <vtkm/filter/multi_block/MergeDataSets.h>
namespace vtkm
{
namespace filter
{
namespace multi_block
{
vtkm::cont::PartitionedDataSet MergeDataSets::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  vtkm::cont::DataSet mergedResult =
    vtkm::cont::MergePartitionedDataSet(input, this->GetInvalidValue());
  return vtkm::cont::PartitionedDataSet(mergedResult);
}
vtkm::cont::DataSet MergeDataSets::DoExecute(const vtkm::cont::DataSet&)
{
  throw vtkm::cont::ErrorFilterExecution("MergeDataSets only works for a PartitionedDataSet");
}
} // namespace multi_block
} // namespace filter
} // namespace vtkm
