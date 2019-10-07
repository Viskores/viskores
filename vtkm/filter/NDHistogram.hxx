//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_NDHistogram_hxx
#define vtk_m_filter_NDHistogram_hxx

#include <vector>
#include <vtkm/cont/DataSet.h>
#include <vtkm/worklet/NDimsHistogram.h>

namespace vtkm
{
namespace filter
{

inline VTKM_CONT NDHistogram::NDHistogram()
{
}

void NDHistogram::AddFieldAndBin(const std::string& fieldName, vtkm::Id numOfBins)
{
  this->FieldNames.push_back(fieldName);
  this->NumOfBins.push_back(numOfBins);
}

vtkm::Float64 NDHistogram::GetBinDelta(size_t fieldIdx)
{
  return BinDeltas[fieldIdx];
}

vtkm::Range NDHistogram::GetDataRange(size_t fieldIdx)
{
  return DataRanges[fieldIdx];
}

template <typename Policy>
inline VTKM_CONT vtkm::cont::DataSet NDHistogram::DoExecute(const vtkm::cont::DataSet& inData,
                                                            vtkm::filter::PolicyBase<Policy> policy)
{
  vtkm::worklet::NDimsHistogram ndHistogram;

  // Set the number of data points
  ndHistogram.SetNumOfDataPoints(inData.GetField(0).GetNumberOfValues());

  // Add field one by one
  // (By using AddFieldAndBin(), the length of FieldNames and NumOfBins must be the same)
  for (size_t i = 0; i < this->FieldNames.size(); i++)
  {
    vtkm::Range rangeField;
    vtkm::Float64 deltaField;
    ndHistogram.AddField(
      vtkm::filter::ApplyPolicyFieldNotActive(inData.GetField(this->FieldNames[i]), policy),
      this->NumOfBins[i],
      rangeField,
      deltaField);
    DataRanges.push_back(rangeField);
    BinDeltas.push_back(deltaField);
  }

  std::vector<vtkm::cont::ArrayHandle<vtkm::Id>> binIds;
  vtkm::cont::ArrayHandle<vtkm::Id> freqs;
  ndHistogram.Run(binIds, freqs);

  vtkm::cont::DataSet outputData;
  for (size_t i = 0; i < binIds.size(); i++)
  {
    outputData.AddField(vtkm::cont::make_FieldPoint(this->FieldNames[i], binIds[i]));
  }
  outputData.AddField(vtkm::cont::make_FieldPoint("Frequency", freqs));

  return outputData;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool NDHistogram::DoMapField(vtkm::cont::DataSet&,
                                              const vtkm::cont::ArrayHandle<T, StorageType>&,
                                              const vtkm::filter::FieldMetadata&,
                                              vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
}
#endif
