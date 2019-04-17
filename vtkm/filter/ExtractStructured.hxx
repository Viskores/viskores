//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ExtractStructured::DoExecute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());
  const vtkm::cont::CoordinateSystem& coordinates =
    input.GetCoordinateSystem(this->GetActiveCellSetIndex());

  auto cellset = this->Worklet.Run(vtkm::filter::ApplyPolicyStructured(cells, policy),
                                   this->VOI,
                                   this->SampleRate,
                                   this->IncludeBoundary);

  auto coords = this->Worklet.MapCoordinates(coordinates);
  vtkm::cont::CoordinateSystem outputCoordinates(coordinates.GetName(), coords);

  vtkm::cont::DataSet output;
  output.AddCellSet(vtkm::cont::DynamicCellSet(cellset));
  output.AddCoordinateSystem(outputCoordinates);
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool ExtractStructured::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  if (fieldMeta.IsPointField())
  {
    vtkm::cont::ArrayHandle<T> output = this->Worklet.ProcessPointField(input);

    result.AddField(fieldMeta.AsField(output));
    return true;
  }

  // cell data must be scattered to the cells created per input cell
  if (fieldMeta.IsCellField())
  {
    vtkm::cont::ArrayHandle<T> output = this->Worklet.ProcessCellField(input);

    result.AddField(fieldMeta.AsField(output));
    return true;
  }

  return false;
}
}
}
