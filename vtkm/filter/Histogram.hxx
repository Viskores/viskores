//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/FieldHistogram.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Histogram::Histogram()
  : NumberOfBins(10)
  , BinDelta(0)
  , DataRange()
{
  this->SetOutputFieldName("histogram");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::filter::Result Histogram::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device)
{
  vtkm::cont::ArrayHandle<vtkm::Id> binArray;
  T delta;

  vtkm::worklet::FieldHistogram worklet;
  worklet.Run(field, this->NumberOfBins, this->DataRange, delta, binArray, device);

  this->BinDelta = static_cast<vtkm::Float64>(delta);
  return vtkm::filter::Result(inDataSet,
                              binArray,
                              this->GetOutputFieldName(),
                              fieldMetadata.GetAssociation(),
                              fieldMetadata.GetCellSetName());
}
}
} // namespace vtkm::filter
