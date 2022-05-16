//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/internal/CastInvalidValue.h>

#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/resampling/Probe.h>
#include <vtkm/filter/resampling/worklet/Probe.h>

namespace vtkm
{
namespace filter
{
namespace resampling
{
vtkm::cont::DataSet Probe::DoExecute(const vtkm::cont::DataSet& input)
{
  vtkm::worklet::Probe worklet;
  worklet.Run(input.GetCellSet(),
              input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
              this->Geometry.GetCoordinateSystem().GetData());

  auto output = this->Geometry;
  auto hpf = worklet.GetHiddenPointsField();
  auto hcf = worklet.GetHiddenCellsField(output.GetCellSet());

  output.AddField(vtkm::cont::make_FieldPoint("HIDDEN", hpf));
  output.AddField(vtkm::cont::make_FieldCell("HIDDEN", hcf));

  auto mapper = [&](auto& outDataSet, const auto& f) { this->DoMapField(outDataSet, f, worklet); };
  this->MapFieldsOntoOutput(input, output, mapper);

  return output;
}


bool Probe::DoMapField(vtkm::cont::DataSet& result,
                       const vtkm::cont::Field& field,
                       const vtkm::worklet::Probe& worklet)
{
  if (field.IsFieldPoint())
  {
    auto resolve = [&](const auto& concrete) {
      using T = typename std::decay_t<decltype(concrete)>::ValueType;
      vtkm::cont::ArrayHandle<T> outputArray = worklet.ProcessPointField(
        concrete, vtkm::cont::internal::CastInvalidValue<T>(this->InvalidValue));
      result.AddPointField(field.GetName(), outputArray);
    };
    // FIXME: what kind of CastAndCall do we need?
    CastAndCall(field.GetData(), resolve);
    return true;
  }
  else if (field.IsFieldCell())
  {
    vtkm::cont::Field outField;
    if (vtkm::filter::MapFieldPermutation(
          field, worklet.GetCellIds(), outField, this->InvalidValue))
    {
      // output field should be associated with points
      outField = vtkm::cont::Field(
        field.GetName(), vtkm::cont::Field::Association::Points, outField.GetData());
      result.AddField(outField);
      return true;
    }
    return false;
  }
  else if (field.IsFieldGlobal())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}
} // namespace resampling
} // namespace filter
} // namespace vtkm
