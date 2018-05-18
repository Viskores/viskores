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

template <typename Policy, typename Device>
inline VTKM_CONT vtkm::cont::DataSet NDEntropy::DoExecute(
  const vtkm::cont::DataSet& inData,
  vtkm::filter::PolicyBase<Policy> vtkmNotUsed(policy),
  Device device)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  vtkm::worklet::NDimsEntropy ndEntropy;
  ndEntropy.SetNumOfDataPoints(inData.GetField(0).GetData().GetNumberOfValues(), device);

  // Add field one by one
  // (By using AddFieldAndBin(), the length of FieldNames and NumOfBins must be the same)
  for (size_t i = 0; i < FieldNames.size(); i++)
  {
    ndEntropy.AddField(inData.GetField(FieldNames[i]).GetData(), NumOfBins[i], device);
  }

  // Run worklet to calculate multi-variate entropy
  vtkm::Float64 entropy = ndEntropy.Run(device);
  vtkm::cont::DataSet outputData;
  outputData.AddField(vtkm::cont::make_Field(
    "Entropy", vtkm::cont::Field::Association::POINTS, &entropy, 1, vtkm::CopyFlag::On));
  return outputData;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool NDEntropy::DoMapField(vtkm::cont::DataSet&,
                                            const vtkm::cont::ArrayHandle<T, StorageType>&,
                                            const vtkm::filter::FieldMetadata&,
                                            const vtkm::filter::PolicyBase<DerivedPolicy>&,
                                            DeviceAdapter)
{
  return false;
}
}
}
