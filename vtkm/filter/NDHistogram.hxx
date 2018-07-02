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

template <typename Policy, typename Device>
inline VTKM_CONT vtkm::cont::DataSet NDHistogram::DoExecute(const vtkm::cont::DataSet& inData,
                                                            vtkm::filter::PolicyBase<Policy> policy,
                                                            Device device)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  vtkm::worklet::NDimsHistogram ndHistogram;

  // Set the number of data points
  ndHistogram.SetNumOfDataPoints(inData.GetField(0).GetData().GetNumberOfValues(), device);

  // Add field one by one
  // (By using AddFieldAndBin(), the length of FieldNames and NumOfBins must be the same)
  for (size_t i = 0; i < FieldNames.size(); i++)
  {
    vtkm::Range rangeField;
    vtkm::Float64 deltaField;
    ndHistogram.AddField(vtkm::filter::ApplyPolicy(inData.GetField(FieldNames[i]), policy),
                         NumOfBins[i],
                         rangeField,
                         deltaField,
                         device);
    DataRanges.push_back(rangeField);
    BinDeltas.push_back(deltaField);
  }

  std::vector<vtkm::cont::ArrayHandle<vtkm::Id>> binIds;
  vtkm::cont::ArrayHandle<vtkm::Id> freqs;
  ndHistogram.Run(binIds, freqs, device);

  vtkm::cont::DataSet outputData;
  for (size_t i = 0; i < binIds.size(); i++)
  {
    outputData.AddField(
      vtkm::cont::Field(FieldNames[i], vtkm::cont::Field::Association::POINTS, binIds[i]));
  }
  outputData.AddField(
    vtkm::cont::Field("Frequency", vtkm::cont::Field::Association::POINTS, freqs));

  return outputData;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool NDHistogram::DoMapField(vtkm::cont::DataSet&,
                                              const vtkm::cont::ArrayHandle<T, StorageType>&,
                                              const vtkm::filter::FieldMetadata&,
                                              const vtkm::filter::PolicyBase<DerivedPolicy>&,
                                              DeviceAdapter)
{
  return false;
}
}
}
