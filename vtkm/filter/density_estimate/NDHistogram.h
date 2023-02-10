//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_density_estimate_NDHistogram_h
#define vtk_m_filter_density_estimate_NDHistogram_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/density_estimate/vtkm_filter_density_estimate_export.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
/// \brief Generate a N-Dims histogram from input fields
///
/// This filter takes a data set and with target fields and bins defined,
/// it would generate a N-Dims histogram from input fields. The result is stored
/// in a field named as "Frequency". This filed contains all the frequencies of
/// the N-Dims histogram in sparse representation. That being said, the result
/// field does not store 0 frequency bins. Meanwhile all input fields now
/// would have the same length and store bin ids instead.
/// E.g. (FieldA[i], FieldB[i], FieldC[i], Frequency[i]) is a bin in the histogram.
/// The first three numbers are binIDs for FieldA, FieldB and FieldC. Frequency[i] stores
/// the frequency for this bin (FieldA[i], FieldB[i], FieldC[i]).
///
class VTKM_FILTER_DENSITY_ESTIMATE_EXPORT NDHistogram : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  void AddFieldAndBin(const std::string& fieldName, vtkm::Id numOfBins);

  // This index is the field position in FieldNames
  // (or the input _fieldName string vector of SetFields() Function)
  VTKM_CONT
  vtkm::Float64 GetBinDelta(size_t fieldIdx);

  // This index is the field position in FieldNames
  // (or the input _fieldName string vector of SetFields() Function)
  VTKM_CONT
  vtkm::Range GetDataRange(size_t fieldIdx);

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  std::vector<vtkm::Id> NumOfBins;
  std::vector<std::string> FieldNames;
  std::vector<vtkm::Float64> BinDeltas;
  std::vector<vtkm::Range> DataRanges; //Min Max of the field
};
} // namespace density_estimate
} // namespace filter
} // namespace vtm

#endif //vtk_m_filter_density_estimate_NDHistogram_h
