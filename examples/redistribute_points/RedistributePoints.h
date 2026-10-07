//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
