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

namespace
{

bool DoMapField(vtkm::cont::DataSet& result,
                const vtkm::cont::Field& field,
                const vtkm::worklet::Probe& worklet,
                vtkm::Float64 invalidValue)
{
  if (field.IsPointField())
  {
    vtkm::cont::UnknownArrayHandle inArray = field.GetData();
    vtkm::cont::UnknownArrayHandle outArray = inArray.NewInstanceBasic();

    bool called = false;
    auto tryType = [&](auto t) {
      using T = std::decay_t<decltype(t)>;
      if (!called && inArray.IsBaseComponentType<T>())
      {
        called = true;
        vtkm::IdComponent numComponents = inArray.GetNumberOfComponentsFlat();
        VTKM_ASSERT(numComponents == outArray.GetNumberOfComponentsFlat());

        for (vtkm::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
        {
          worklet.ProcessPointField(inArray.ExtractComponent<T>(cIndex),
                                    outArray.ExtractComponent<T>(cIndex, vtkm::CopyFlag::Off),
                                    vtkm::cont::internal::CastInvalidValue<T>(invalidValue));
        }
      }
    };
    vtkm::ListForEach(tryType, vtkm::TypeListScalarAll{});
    if (!called)
    {
      VTKM_LOG_CAST_FAIL(worklet, vtkm::TypeListScalarAll);
      return false;
    }

    result.AddPointField(field.GetName(), outArray);
    return true;
  }
  else if (field.IsCellField())
  {
    vtkm::cont::Field outField;
    if (vtkm::filter::MapFieldPermutation(field, worklet.GetCellIds(), outField, invalidValue))
    {
      // output field should be associated with points
      outField = vtkm::cont::Field(
        field.GetName(), vtkm::cont::Field::Association::Points, outField.GetData());
      result.AddField(outField);
      return true;
    }
    return false;
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}
} // anonymous namespace

vtkm::cont::DataSet Probe::DoExecute(const vtkm::cont::DataSet& input)
{
  vtkm::worklet::Probe worklet;
  worklet.Run(input.GetCellSet(),
              input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
              this->Geometry.GetCoordinateSystem().GetData());

  auto mapper = [&](auto& outDataSet, const auto& f) {
    DoMapField(outDataSet, f, worklet, this->InvalidValue);
  };
  auto output = this->CreateResultCoordinateSystem(
    input, this->Geometry.GetCellSet(), this->Geometry.GetCoordinateSystem(), mapper);
  output.AddField(vtkm::cont::make_FieldPoint("HIDDEN", worklet.GetHiddenPointsField()));
  output.AddField(
    vtkm::cont::make_FieldCell("HIDDEN", worklet.GetHiddenCellsField(output.GetCellSet())));

  return output;
}

} // namespace resampling
} // namespace filter
} // namespace vtkm
