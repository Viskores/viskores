//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT ExtractStructured::ExtractStructured()
  : vtkm::filter::FilterDataSet<ExtractStructured>()
  , VOI(vtkm::Bounds(1, 1, 1, 1, 1, 1))
  , SampleRate(vtkm::Id3(1, 1, 1))
  , IncludeBoundary(false)
  , Worklet()
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::filter::ResultDataSet ExtractStructured::DoExecute(
  const vtkm::cont::DataSet& input, const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter&)
{
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());
  const vtkm::cont::CoordinateSystem& coordinates =
    input.GetCoordinateSystem(this->GetActiveCellSetIndex());

  vtkm::cont::DataSet output = this->Worklet.Run(cells, coordinates, this->VOI, this->SampleRate,
                                                 this->IncludeBoundary, DeviceAdapter());

  return vtkm::filter::ResultDataSet(output);
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool ExtractStructured::DoMapField(
  vtkm::filter::ResultDataSet& result, const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta, const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device)
{
  // point data is copied as is because it was not collapsed
  if (fieldMeta.IsPointField())
  {
    vtkm::cont::ArrayHandle<T, StorageType> output = this->Worklet.ProcessPointField(input, device);

    result.GetDataSet().AddField(fieldMeta.AsField(output));
    return true;
  }

  // cell data must be scattered to the cells created per input cell
  if (fieldMeta.IsCellField())
  {
    vtkm::cont::ArrayHandle<T, StorageType> output = this->Worklet.ProcessCellField(input, device);

    result.GetDataSet().AddField(fieldMeta.AsField(output));
    return true;
  }

  return false;
}
}
}
