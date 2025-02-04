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

#include <viskores/filter/Filter.h>

namespace example
{

class RedistributePoints : public viskores::filter::Filter
{
public:
  VISKORES_CONT RedistributePoints() {}

  VISKORES_CONT ~RedistributePoints() {}

protected:
  VISKORES_CONT viskores::cont::PartitionedDataSet DoExecutePartitions(
    const viskores::cont::PartitionedDataSet& input) override;

  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace example

#endif //example_RedistributePoints_h
