//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_density_estimate_Histogram_h
#define vtk_m_filter_density_estimate_Histogram_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/density_estimate/vtkm_filter_density_estimate_export.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
/// \brief Construct the histogram of a given field.
///
/// The range of the field is evenly split to a set number of bins (set by
/// `SetNumberOfBins()`). This filter then counts the number of values in the filter
/// that are in each bin.
///
/// The result of this filter is stored in a `vtkm::cont::DataSet` with no points
/// or cells. It contains only a single field containing the histogram (bin counts).
/// The field has an association of `vtkm::cont::Field::Association::WholeDataSet`.
/// The field contains an array of `vtkm::Id` with the bin counts. By default, the
/// field is named "histogram", but that can be changed with the `SetOutputFieldName()`
/// method.
///
/// If this filter is run on a partitioned data set, the result will be a
/// `vtkm::cont::PartitionedDataSet` containing a single
/// `vtkm::cont::DataSet` as previously described.
///
class VTKM_FILTER_DENSITY_ESTIMATE_EXPORT Histogram : public vtkm::filter::Filter
{
public:
  VTKM_CONT Histogram();

  /// @brief Set the number of bins for the resulting histogram.
  ///
  /// By default, a histogram with 10 bins is created.
  VTKM_CONT void SetNumberOfBins(vtkm::Id count) { this->NumberOfBins = count; }

  /// @brief Get the number of bins for the resulting histogram.
  VTKM_CONT vtkm::Id GetNumberOfBins() const { return this->NumberOfBins; }

  /// @brief Set the range to use to generate the histogram.
  ///
  /// If range is set to empty, the field's global range (computed using
  /// `vtkm::cont::FieldRangeGlobalCompute`) will be used.
  VTKM_CONT void SetRange(const vtkm::Range& range) { this->Range = range; }

  /// @brief Get the range used to generate the histogram.
  ///
  /// If the returned range is empty, then the field's global range will be used.
  VTKM_CONT const vtkm::Range& GetRange() const { return this->Range; }

  /// @brief Returns the size of bin in the computed histogram.
  ///
  /// This value is only valid after a call to `Execute`.
  VTKM_CONT vtkm::Float64 GetBinDelta() const { return this->BinDelta; }

  /// @brief Returns the range used for most recent execute.
  ///
  /// If `SetRange` is used to specify a non-empty range, then this range will
  /// be returned. Otherwise, the coputed range is returned.
  /// This value is only valid after a call to `Execute`.
  VTKM_CONT vtkm::Range GetComputedRange() const { return this->ComputedRange; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData) override;

  ///@{
  /// when operating on vtkm::cont::PartitionedDataSet, we
  /// want to do processing across ranks as well. Just adding pre/post handles
  /// for the same does the trick.
  VTKM_CONT void PreExecute(const vtkm::cont::PartitionedDataSet& input);
  VTKM_CONT void PostExecute(const vtkm::cont::PartitionedDataSet& input,
                             vtkm::cont::PartitionedDataSet& output);
  ///@}

  vtkm::Id NumberOfBins = 10;
  vtkm::Float64 BinDelta = 0;
  vtkm::Range ComputedRange;
  vtkm::Range Range;
  bool InExecutePartitions = false;
};
} // namespace density_estimate
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_density_estimate_Histogram_h
