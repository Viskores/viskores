//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_examples_histogram_HistogramMPI_h
#define vtk_m_examples_histogram_HistogramMPI_h

#include <vtkm/filter/FilterField.h>

namespace example
{

/// \brief Construct the HistogramMPI of a given Field
///
/// Construct a HistogramMPI with a default of 10 bins.
///
class HistogramMPI : public vtkm::filter::FilterField<HistogramMPI>
{
public:
  //currently the HistogramMPI filter only works on scalar data.
  //this mainly has to do with getting the ranges for each bin
  //would require returning a more complex value type
  using SupportedTypes = vtkm::TypeListScalarAll;

  //Construct a HistogramMPI with a default of 10 bins
  VTKM_CONT
  HistogramMPI();

  VTKM_CONT
  void SetNumberOfBins(vtkm::Id count) { this->NumberOfBins = count; }

  VTKM_CONT
  vtkm::Id GetNumberOfBins() const { return this->NumberOfBins; }

  //@{
  /// Get/Set the range to use to generate the HistogramMPI. If range is set to
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
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

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
} // namespace example

#include "HistogramMPI.hxx"

#endif // vtk_m_filter_Histogram_h
