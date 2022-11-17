//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/entity_extraction/Threshold.h>
#include <vtkm/filter/entity_extraction/worklet/Threshold.h>

#include <vtkm/BinaryPredicates.h>
#include <vtkm/Math.h>

namespace
{
class ThresholdRange
{
public:
  VTKM_CONT ThresholdRange() = default;

  VTKM_CONT
  ThresholdRange(const vtkm::Float64& lower, const vtkm::Float64& upper)
    : Lower(lower)
    , Upper(upper)
  {
  }

  template <typename T>
  VTKM_EXEC bool operator()(const T& value) const
  {
    return static_cast<vtkm::Float64>(value) >= this->Lower &&
      static_cast<vtkm::Float64>(value) <= this->Upper;
  }

private:
  vtkm::Float64 Lower;
  vtkm::Float64 Upper;
};

bool DoMapField(vtkm::cont::DataSet& result,
                const vtkm::cont::Field& field,
                const vtkm::worklet::Threshold& worklet)
{
  if (field.IsPointField() || field.IsWholeDataSetField())
  {
    //we copy the input handle to the result dataset, reusing the metadata
    result.AddField(field);
    return true;
  }
  else if (field.IsCellField())
  {
    return vtkm::filter::MapFieldPermutation(field, worklet.GetValidCellIds(), result);
  }
  else
  {
    return false;
  }
}
} // end anon namespace

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
//-----------------------------------------------------------------------------
VTKM_CONT void Threshold::SetThresholdBelow(vtkm::Float64 value)
{
  this->SetLowerThreshold(vtkm::NegativeInfinity<vtkm::Float64>());
  this->SetUpperThreshold(value);
}

VTKM_CONT void Threshold::SetThresholdAbove(vtkm::Float64 value)
{
  this->SetLowerThreshold(value);
  this->SetUpperThreshold(vtkm::Infinity<vtkm::Float64>());
}

VTKM_CONT void Threshold::SetThresholdBetween(vtkm::Float64 value1, vtkm::Float64 value2)
{
  this->SetLowerThreshold(value1);
  this->SetUpperThreshold(value2);
}

//-----------------------------------------------------------------------------
vtkm::cont::DataSet Threshold::DoExecute(const vtkm::cont::DataSet& input)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  const auto& field = this->GetFieldFromDataSet(input);

  ThresholdRange predicate(this->GetLowerThreshold(), this->GetUpperThreshold());
  vtkm::worklet::Threshold worklet;
  vtkm::cont::UnknownCellSet cellOut;

  auto callWithArrayBaseComponent = [&](auto baseComp) {
    using ComponentType = decltype(baseComp);
    if (!field.GetData().IsBaseComponentType<ComponentType>())
    {
      return;
    }

    if (this->ComponentMode == Component::Selected)
    {
      auto arrayComponent =
        field.GetData().ExtractComponent<ComponentType>(this->SelectedComponent);
      cellOut = worklet.Run(
        cells, arrayComponent, field.GetAssociation(), predicate, this->AllInRange, this->Invert);
    }
    else
    {
      for (vtkm::IdComponent i = 0; i < field.GetData().GetNumberOfComponents(); ++i)
      {
        auto arrayComponent = field.GetData().ExtractComponent<ComponentType>(i);
        if (this->ComponentMode == Component::Any)
        {
          worklet.RunIncremental(cells,
                                 arrayComponent,
                                 field.GetAssociation(),
                                 predicate,
                                 this->AllInRange,
                                 vtkm::LogicalOr{});
        }
        else // this->ComponentMode == Component::All
        {
          worklet.RunIncremental(cells,
                                 arrayComponent,
                                 field.GetAssociation(),
                                 predicate,
                                 this->AllInRange,
                                 vtkm::LogicalAnd{});
        }
      }

      if (this->Invert)
      {
        worklet.InvertResults();
      }

      cellOut = worklet.GenerateResultCellSet(cells);
    }
  };

  vtkm::ListForEach(callWithArrayBaseComponent, vtkm::TypeListScalarAll{});

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, cellOut, mapper);
}
} // namespace entity_extraction
} // namespace filter
} // namespace vtkm
