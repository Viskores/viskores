//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vector>

#include <vtkm/filter/density_estimate/NDHistogram.h>
#include <vtkm/filter/density_estimate/worklet/NDimsHistogram.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
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

VTKM_CONT vtkm::cont::DataSet NDHistogram::DoExecute(const vtkm::cont::DataSet& inData)
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
      inData.GetField(this->FieldNames[i]).GetData(), this->NumOfBins[i], rangeField, deltaField);
    DataRanges.push_back(rangeField);
    BinDeltas.push_back(deltaField);
  }

  std::vector<vtkm::cont::ArrayHandle<vtkm::Id>> binIds;
  vtkm::cont::ArrayHandle<vtkm::Id> freqs;
  ndHistogram.Run(binIds, freqs);

  vtkm::cont::DataSet outputData;
  for (size_t i = 0; i < binIds.size(); i++)
  {
    outputData.AddField(
      { this->FieldNames[i], vtkm::cont::Field::Association::WholeDataSet, binIds[i] });
  }
  outputData.AddField({ "Frequency", vtkm::cont::Field::Association::WholeDataSet, freqs });
  // The output is a "summary" of the input, no need to map fields
  return outputData;
}

} // namespace density_estimate
} // namespace filter
} // namespace vtkm
