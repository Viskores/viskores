//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSet.h>
#include <vtkm/worklet/NDimsEntropy.h>

namespace vtkm
{
namespace filter
{

inline VTKM_CONT NDEntropy::NDEntropy()
{
}

void NDEntropy::AddFieldAndBin(const std::string& fieldName, vtkm::Id numOfBins)
{
  this->FieldNames.push_back(fieldName);
  this->NumOfBins.push_back(numOfBins);
}

template <typename Policy>
inline VTKM_CONT vtkm::cont::DataSet NDEntropy::DoExecute(
  const vtkm::cont::DataSet& inData,
  vtkm::filter::PolicyBase<Policy> vtkmNotUsed(policy))
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
  vtkm::Float64 entropy = ndEntropy.Run();
  vtkm::cont::DataSet outputData;
  outputData.AddField(vtkm::cont::make_Field(
    "Entropy", vtkm::cont::Field::Association::POINTS, &entropy, 1, vtkm::CopyFlag::On));
  return outputData;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool NDEntropy::DoMapField(vtkm::cont::DataSet&,
                                            const vtkm::cont::ArrayHandle<T, StorageType>&,
                                            const vtkm::filter::FieldMetadata&,
                                            vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
}
