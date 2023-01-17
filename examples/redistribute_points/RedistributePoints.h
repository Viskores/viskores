//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef example_RedistributePoints_h
#define example_RedistributePoints_h

#include <vtkm/filter/Filter.h>

namespace example
{

class RedistributePoints : public vtkm::filter::Filter
{
public:
  VTKM_CONT RedistributePoints() {}

  VTKM_CONT ~RedistributePoints() {}

protected:
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& input) override;

  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};

} // namespace example

#endif //example_RedistributePoints_h
