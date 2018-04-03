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

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT ExtractStructured::ExtractStructured()
  : vtkm::filter::FilterDataSet<ExtractStructured>()
  , VOI(vtkm::RangeId3(0, -1, 0, -1, 0, -1))
  , SampleRate(vtkm::Id3(1, 1, 1))
  , IncludeBoundary(false)
  , Worklet()
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet ExtractStructured::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& device)
{
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());
  const vtkm::cont::CoordinateSystem& coordinates =
    input.GetCoordinateSystem(this->GetActiveCellSetIndex());

  auto cellset = this->Worklet.Run(vtkm::filter::ApplyPolicyStructured(cells, policy),
                                   this->VOI,
                                   this->SampleRate,
                                   this->IncludeBoundary,
                                   device);

  auto coords = this->Worklet.MapCoordinates(coordinates, device);
  vtkm::cont::CoordinateSystem outputCoordinates(coordinates.GetName(), coords);

  vtkm::cont::DataSet output;
  output.AddCellSet(vtkm::cont::DynamicCellSet(cellset));
  output.AddCoordinateSystem(outputCoordinates);
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool ExtractStructured::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device)
{
  if (fieldMeta.IsPointField())
  {
    vtkm::cont::ArrayHandle<T> output = this->Worklet.ProcessPointField(input, device);

    result.AddField(fieldMeta.AsField(output));
    return true;
  }

  // cell data must be scattered to the cells created per input cell
  if (fieldMeta.IsCellField())
  {
    vtkm::cont::ArrayHandle<T> output = this->Worklet.ProcessCellField(input, device);

    result.AddField(fieldMeta.AsField(output));
    return true;
  }

  return false;
}
}
}
