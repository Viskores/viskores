//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/density_estimate/NDEntropy.h>
#include <vtkm/filter/density_estimate/worklet/NDimsEntropy.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
void NDEntropy::AddFieldAndBin(const std::string& fieldName, vtkm::Id numOfBins)
{
  this->FieldNames.push_back(fieldName);
  this->NumOfBins.push_back(numOfBins);
}

VTKM_CONT vtkm::cont::DataSet NDEntropy::DoExecute(const vtkm::cont::DataSet& inData)
{
  vtkm::worklet::NDimsEntropy ndEntropy;
  ndEntropy.SetNumOfDataPoints(inData.GetField(0).GetNumberOfValues());

  // Add field one by one
  // (By using AddFieldAndBin(), the length of FieldNames and NumOfBins must be the same)
  for (size_t i = 0; i < FieldNames.size(); i++)
  {
    ndEntropy.AddField(inData.GetField(FieldNames[i]).GetData(), NumOfBins[i]);
  }

  // Run worklet to calculate multi-variate entropy
  vtkm::cont::ArrayHandle<vtkm::Float64> entropyHandle;
  vtkm::Float64 entropy = ndEntropy.Run();

  entropyHandle.Allocate(1);
  entropyHandle.WritePortal().Set(0, entropy);

  vtkm::cont::DataSet outputData;
  outputData.AddField({ "Entropy", vtkm::cont::Field::Association::WholeMesh, entropyHandle });
  // The output is a "summary" of the input, no need to map fields
  return outputData;
}
} // namespace density_estimate
} // namespace filter
} // namespace vtkm
