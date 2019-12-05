//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Histogram_h
#define vtk_m_filter_Histogram_h

#include <vtkm/filter/FilterField.h>

namespace vtkm
{
namespace filter
{

/// \brief Construct the histogram of a given Field
///
/// Construct a histogram with a default of 10 bins.
///
class Histogram : public vtkm::filter::FilterField<Histogram>
{
public:
  using SupportedTypes = vtkm::TypeListScalarAll;

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

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  //@{
  /// when operating on vtkm::cont::PartitionedDataSet, we
  /// want to do processing across ranks as well. Just adding pre/post handles
  /// for the same does the trick.
  template <typename DerivedPolicy>
  VTKM_CONT void PreExecute(const vtkm::cont::PartitionedDataSet& input,
                            const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  template <typename DerivedPolicy>
  VTKM_CONT void PostExecute(const vtkm::cont::PartitionedDataSet& input,
                             vtkm::cont::PartitionedDataSet& output,
                             const vtkm::filter::PolicyBase<DerivedPolicy>&);
  //@}

private:
  vtkm::Id NumberOfBins;
  vtkm::Float64 BinDelta;
  vtkm::Range ComputedRange;
  vtkm::Range Range;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Histogram.hxx>

#endif // vtk_m_filter_Histogram_h
