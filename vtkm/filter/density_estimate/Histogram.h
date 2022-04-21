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

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/density_estimate/vtkm_filter_density_estimate_export.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
/// \brief Construct the histogram of a given Field
///
/// Construct a histogram with a default of 10 bins.
///
class VTKM_FILTER_DENSITY_ESTIMATE_EXPORT Histogram : public vtkm::filter::NewFilterField
{
public:
  //Construct a histogram with a default of 10 bins
  VTKM_CONT
  Histogram();

  VTKM_CONT
  void SetNumberOfBins(vtkm::Id count) { this->NumberOfBins = count; }

  VTKM_CONT
  vtkm::Id GetNumberOfBins() const { return this->NumberOfBins; }

  //@{
  /// Get/Set the range to use to generate the histogram. If range is set to
  /// empty, the field's global range (computed using `vtkm::cont::FieldRangeGlobalCompute`)
  /// will be used.
  VTKM_CONT
  void SetRange(const vtkm::Range& range) { this->Range = range; }

  VTKM_CONT
  const vtkm::Range& GetRange() const { return this->Range; }
  //@}

  /// Returns the bin delta of the last computed field.
  VTKM_CONT
  vtkm::Float64 GetBinDelta() const { return this->BinDelta; }

  /// Returns the range used for most recent execute. If `SetRange` is used to
  /// specify and non-empty range, then this will be same as the range after
  /// the `Execute` call.
  VTKM_CONT
  vtkm::Range GetComputedRange() const { return this->ComputedRange; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData) override;

  //@{
  /// when operating on vtkm::cont::PartitionedDataSet, we
  /// want to do processing across ranks as well. Just adding pre/post handles
  /// for the same does the trick.
  VTKM_CONT void PreExecute(const vtkm::cont::PartitionedDataSet& input);
  VTKM_CONT void PostExecute(const vtkm::cont::PartitionedDataSet& input,
                             vtkm::cont::PartitionedDataSet& output);
  //@}

  vtkm::Id NumberOfBins = 10;
  vtkm::Float64 BinDelta = 0;
  vtkm::Range ComputedRange;
  vtkm::Range Range;
  bool InExecutePartitions = false;
};
} // namespace density_estimate
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::density_estimate::Histogram.") Histogram
  : public vtkm::filter::density_estimate::Histogram
{
  using density_estimate::Histogram::Histogram;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_density_estimate_Histogram_h
