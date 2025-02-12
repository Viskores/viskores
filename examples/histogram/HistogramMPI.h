//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_examples_histogram_HistogramMPI_h
#define viskores_examples_histogram_HistogramMPI_h

#include <viskores/filter/FilterField.h>

namespace example
{

/// \brief Construct the HistogramMPI of a given Field
///
/// Construct a HistogramMPI with a default of 10 bins.
///
class HistogramMPI : public viskores::filter::FilterField
{
public:
  //currently the HistogramMPI filter only works on scalar data.
  //this mainly has to do with getting the ranges for each bin
  //would require returning a more complex value type
  using SupportedTypes = viskores::TypeListScalarAll;

  //Construct a HistogramMPI with a default of 10 bins
  VISKORES_CONT
  HistogramMPI() { this->SetOutputFieldName("histogram"); }

  VISKORES_CONT
  void SetNumberOfBins(viskores::Id count) { this->NumberOfBins = count; }

  VISKORES_CONT
  viskores::Id GetNumberOfBins() const { return this->NumberOfBins; }

  ///@{
  /// Get/Set the range to use to generate the HistogramMPI. If range is set to
  /// empty, the field's global range (computed using `viskores::cont::FieldRangeGlobalCompute`)
  /// will be used.
  VISKORES_CONT
  void SetRange(const viskores::Range& range) { this->Range = range; }

  VISKORES_CONT
  const viskores::Range& GetRange() const { return this->Range; }
  ///@}

  /// Returns the bin delta of the last computed field.
  VISKORES_CONT
  viskores::Float64 GetBinDelta() const { return this->BinDelta; }

  /// Returns the range used for most recent execute. If `SetRange` is used to
  /// specify and non-empty range, then this will be same as the range after
  /// the `Execute` call.
  VISKORES_CONT
  viskores::Range GetComputedRange() const { return this->ComputedRange; }

protected:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
  VISKORES_CONT viskores::cont::PartitionedDataSet DoExecutePartitions(
    const viskores::cont::PartitionedDataSet& input) override;

  ///@{
  /// when operating on viskores::cont::PartitionedDataSet, we
  /// want to do processing across ranks as well. Just adding pre/post handles
  /// for the same does the trick.
  VISKORES_CONT void PreExecute(const viskores::cont::PartitionedDataSet& input);
  VISKORES_CONT void PostExecute(const viskores::cont::PartitionedDataSet& input,
                             viskores::cont::PartitionedDataSet& output);
  ///@}

private:
  viskores::Id NumberOfBins = 10;
  viskores::Float64 BinDelta = 0;
  viskores::Range ComputedRange;
  viskores::Range Range;
  bool InExecutePartitions = false;
};
} // namespace example

#endif // viskores_filter_Histogram_h
