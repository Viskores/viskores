//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_filter_NDHistogram_h
#define vtk_m_filter_NDHistogram_h

#include <vtkm/filter/FilterDataSet.h>

namespace vtkm
{
namespace filter
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
class NDHistogram : public vtkm::filter::FilterDataSet<NDHistogram>
{
public:
  VTKM_CONT
  NDHistogram();

  VTKM_CONT
  void AddFieldAndBin(const std::string& fieldName, vtkm::Id numOfBins);

  template <typename Policy, typename Device>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData,
                                          vtkm::filter::PolicyBase<Policy> policy,
                                          Device);

  // This index is the field position in FieldNames
  // (or the input _fieldName string vector of SetFields() Function)
  VTKM_CONT
  vtkm::Float64 GetBinDelta(size_t fieldIdx);

  // This index is the field position in FieldNames
  // (or the input _fieldName string vector of SetFields() Function)
  VTKM_CONT
  vtkm::Range GetDataRange(size_t fieldIdx);

  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                            DeviceAdapter tag);

private:
  std::vector<vtkm::Id> NumOfBins;
  std::vector<std::string> FieldNames;
  std::vector<vtkm::Float64> BinDeltas;
  std::vector<vtkm::Range> DataRanges; //Min Max of the field
};
}
} // namespace vtkm::filter

#include <vtkm/filter/NDHistogram.hxx>

#endif //vtk_m_filter_NDHistogram_h
